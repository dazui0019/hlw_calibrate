#pragma once

#include <stdint.h>

int evse_db_get_hlw_param(uint32_t VparamXK[], uint32_t IparamXK[], uint32_t PparamXK[]);
int evse_db_set_hlw_param(uint32_t VparamXK[], uint32_t IparamXK[], uint32_t PparamXK[]);

int evse_db_get_calibrated(uint32_t *calibrated);
int evse_db_set_calibrated(uint32_t calibrated);
