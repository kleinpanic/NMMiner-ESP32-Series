import json
from pathlib import Path
import socket
import sys
import threading
import unittest

sys.path.insert(0, str(Path(__file__).parents[1] / "scripts"))
from check_miner_port_reuse import request


class HttpFixture:
    def __init__(self, body, *, length=True, truncate=False, close_first=False):
        size = len(body) + int(truncate)
        header = b"HTTP/1.0 200 OK\r\nConnection: close\r\n"
        if length:
            header += f"Content-Length: {size}\r\n".encode()
        self.response = header + b"\r\n" + body
        self.close_first = close_first
        self.client_closed = False
        self.error = None

    def __enter__(self):
        self.listener = socket.socket()
        self.listener.bind(("127.0.0.1", 0))
        self.listener.listen(1)
        self.listener.settimeout(3)
        self.port = self.listener.getsockname()[1]
        self.worker = threading.Thread(target=self.serve, daemon=True)
        self.worker.start()
        return self

    def serve(self):
        try:
            client, _ = self.listener.accept()
            with client:
                client.settimeout(3)
                data = b""
                while b"\r\n\r\n" not in data:
                    chunk = client.recv(512)
                    if not chunk:
                        raise ValueError("Client failed to send a request")
                    data += chunk
                client.sendall(self.response)
                if self.close_first:
                    client.shutdown(socket.SHUT_WR)
                try:
                    self.client_closed = client.recv(1) == b""
                except ConnectionResetError:
                    self.client_closed = True
        except Exception as error:
            self.error = error

    def __exit__(self, *_):
        self.worker.join(4)
        self.listener.close()
        if self.worker.is_alive():
            raise AssertionError("Test connection did not release")
        if self.error:
            raise self.error
        if not self.client_closed:
            raise AssertionError("Checker leaked its client socket")


class PortReuseCheckerTests(unittest.TestCase):
    def check(self, fixture, path="/api/status", **kwargs):
        return request("127.0.0.1", "127.0.0.1", 0, path,
                       server_port=fixture.port, **kwargs)

    def test_complete_json_closes_client_and_omits_wallet_values(self):
        body = json.dumps({"miner": {"poolAuthorized": True},
                           "wallet": {"balanceSats": 1729384756}}).encode()
        with HttpFixture(body) as server:
            result = self.check(server)
        self.assertEqual(result["status"], "PASS")
        self.assertEqual(result["bytes"], len(body))
        self.assertNotIn("1729384756", json.dumps(result))
        self.assertNotIn("wallet", result)

    def test_csv_checks_all_nine_columns(self):
        with HttpFixture(b"a,b,c,d,e,f,g,h,i\n1,2,3,4,5,6,7,8,9\n") as server:
            self.assertEqual(self.check(server, "/api/sd/log")["rows"], 1)
        with HttpFixture(b"a,b,c,d,e,f,g,h,i\n1,2\n") as server:
            with self.assertRaises(ValueError):
                self.check(server, "/api/sd/log")

    def test_truncation_missing_length_and_unauthorized_pool_fail(self):
        body = b'{"miner":{"poolAuthorized":true}}'
        for kwargs in ({"truncate": True}, {"length": False}):
            with self.subTest(kwargs=kwargs), HttpFixture(body, close_first=True, **kwargs) as server:
                with self.assertRaises(ValueError):
                    self.check(server)
        with HttpFixture(b'{"miner":{"poolAuthorized":false}}') as server:
            with self.assertRaises(ValueError):
                self.check(server)

    def test_eof_wait_records_server_close(self):
        with HttpFixture(b'{"miner":{"poolAuthorized":true}}', close_first=True) as server:
            result = self.check(server, wait_eof=True)
        self.assertGreaterEqual(result["eof_wait_seconds"], 0)
        self.assertLess(result["eof_wait_seconds"], 4)


if __name__ == "__main__":
    unittest.main()
