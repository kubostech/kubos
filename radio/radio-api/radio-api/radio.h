/*
 * Copyright (C) 2017 Kubos Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * @defgroup radio Kubos Radio Interface
 * @addtogroup radio
 * @{
 */

#pragma once

#include <stdint.h>

/**
 * Radio function return values
 */
typedef enum {
    /** Function call completed successfully */
    RADIO_OK = 0,
    /** Radio receive buffer is empty */
    RADIO_RX_EMPTY,
    /** Generic radio error */
    RADIO_ERROR,
    /** Function input parameter is invalid */
    RADIO_ERROR_CONFIG
} KRadioStatus;

/**
 * Radio reset types
 */
typedef enum {
    /** Perform hardware-level radio reset */
    RADIO_HARD_RESET,
    /** Perform software radio reset */
    RADIO_SOFT_RESET
} KRadioReset;

/**
 * AX.25 call-sign structure
 */
typedef struct
{
    /**
     * Six character station call-sign
     */
    uint8_t ascii[6];
    /**
     * One byte station SSID value
     */
    uint8_t ssid;
} ax25_callsign;

/**
 * Initialize the radio interface
 * @return KRadioStatus RADIO_OK if OK, error otherwise
 */
KRadioStatus k_radio_init(void);
/**
 * Terminate the radio interface
 */
void k_radio_terminate(void);
/**
 * Configure the radio
 * @note This function might not be implemented for all radios. See specific radio API documentation for configuration availability and structure
 * @param [in] radio_config Pointer to the radio configuration structure
 * @return KRadioStatus RADIO_OK if OK, error otherwise
 */
KRadioStatus k_radio_configure(uint8_t * radio_config);
/**
 * Reset the radio
 * @note This function might not be implemented for all radios
 * @param [in] type Type of reset to perform (hard, soft, etc)
 * @return KRadioStatus RADIO_OK if OK, error otherwise
 */
KRadioStatus k_radio_reset(uint8_t type);
/**
 * Send a message to the radio's transmit buffer
 * @param [in] buffer Pointer to the message to send
 * @param [in] len Length of the message to send
 * @return uint8_t See specific radio API documentation for return code documentation
 */
uint8_t k_radio_send(char * buffer, int len);
/**
 * Receive a message from the radio's receive buffer
 * @param [in] buffer Pointer where the message should be copied to
 * @param [out] len Length of the received message
 * @return KRadioStatus RADIO_OK if a message was received successfully, RADIO_RX_EMPTY if there are no messages to receive, error otherwise
 */
KRadioStatus k_radio_recv(char * buffer, uint8_t * len);
/**
 * Read radio telemetry values
 * @note See specific radio API documentation for available telemetry types
 * @param [in] buffer Pointer to structure which data should be copied to
 * @param [in] type Telemetry packet to read
 * @return KRadioStatus RADIO_OK if OK, error otherwise
 */
KRadioStatus k_radio_get_telemetry(uint8_t * buffer, uint8_t type);

/* @} */
