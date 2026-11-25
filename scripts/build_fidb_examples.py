#!/usr/bin/env python3
import os
import subprocess
import shutil

IDF_PATH = "/home/broklein/esp/esp-idf"
EXAMPLES = [
    "examples/storage/nvs_rw_value",
    "examples/wifi/getting_started/station",
    "examples/protocols/http_server/simple",
    "examples/peripherals/ledc/ledc_basic",
    "examples/peripherals/spi_master/lcd",
    "examples/get-started/hello_world"
]

OUT_DIR = "/home/broklein/hardware/recovered-assets/fidb_examples"

def run_cmd(cmd, cwd=None, env=None):
    print(f"Running: {cmd} in {cwd}")
    # We need to run in a shell to support 'source' if needed, 
    # but here we use the idf_tools.py export output style
    result = subprocess.run(cmd, shell=True, cwd=cwd, env=env, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"Error: {result.stderr}")
    return result

def main():
    if not os.path.exists(OUT_DIR):
        os.makedirs(OUT_DIR)

    # Get environment variables from export.py
    export_cmd = f"python3 {IDF_PATH}/tools/idf_tools.py export --format shell"
    export_proc = subprocess.run(export_cmd, shell=True, capture_output=True, text=True)
    
    env = os.environ.copy()
    # Ensure standard bin paths are included
    standard_paths = ["/usr/bin", "/bin", "/usr/local/bin"]
    current_path = env.get("PATH", "")
    for p in standard_paths:
        if p not in current_path:
            current_path = p + os.pathsep + current_path
    env["PATH"] = current_path

    # The output might be a single line with multiple export commands separated by semicolons
    commands = export_proc.stdout.replace("\n", "").split(";")
    for cmd in commands:
        if cmd.strip().startswith("export "):
            key_val = cmd.strip()[7:].split("=", 1)
            if len(key_val) == 2:
                key, val = key_val
                # Expand variables like $PATH
                val = val.replace("$PATH", env.get("PATH", ""))
                env[key] = val
    
    env["IDF_PATH"] = IDF_PATH
    env["CMAKE_POLICY_VERSION_MINIMUM"] = "3.5"

    idf_py = os.path.join(env["IDF_PYTHON_ENV_PATH"], "bin", "python") + " " + os.path.join(IDF_PATH, "tools", "idf.py")

    for ex in EXAMPLES:
        ex_path = os.path.join(IDF_PATH, ex)
        ex_name = os.path.basename(ex)
        
        for opt in ["Os", "O2"]:
            print(f"Building {ex} with -{opt}...")
            
            # Clean build dir
            build_dir = os.path.join(ex_path, "build")
            if os.path.exists(build_dir):
                shutil.rmtree(build_dir, ignore_errors=True)
            
            # Set optimization level
            # In 4.4.6, these are the typical config names
            opt_flag = ""
            if opt == "Os":
                opt_flag = "-DCONFIG_COMPILER_OPTIMIZATION_SIZE=y"
            else:
                opt_flag = "-DCONFIG_COMPILER_OPTIMIZATION_PERF=y"
                
            cmd = f"{idf_py} set-target esp32 && {idf_py} {opt_flag} -DCMAKE_POLICY_VERSION_MINIMUM=3.5 build"
            res = run_cmd(cmd, cwd=ex_path, env=env)
            
            if res.returncode == 0:
                elf_src = os.path.join(build_dir, f"{ex_name}.elf")
                if os.path.exists(elf_src):
                    dest = os.path.join(OUT_DIR, f"{ex_name}_{opt}.elf")
                    shutil.copy(elf_src, dest)
                    print(f"Copied {elf_src} to {dest}")
                else:
                    print(f"Warning: ELF not found at {elf_src}")
            else:
                print(f"Build failed for {ex} -{opt}")

if __name__ == "__main__":
    main()
