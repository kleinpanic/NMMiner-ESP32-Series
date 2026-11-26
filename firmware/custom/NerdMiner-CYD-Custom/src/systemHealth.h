#pragma once
void systemHealthStart();
// Percentage of scheduler samples outside each core's idle task.
bool systemCpuUsage(float out[2]);
