#include "ambient_tx.h"

#include <string.h>

#include "ambient.h"

static void tx_header_init(FDCAN_TxHeaderTypeDef *tx_header, uint32_t id)
{
    tx_header->Identifier = id;
    tx_header->IdType = FDCAN_STANDARD_ID;
    tx_header->TxFrameType = FDCAN_DATA_FRAME;
    tx_header->DataLength = FDCAN_DLC_BYTES_8;
    tx_header->ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx_header->BitRateSwitch = FDCAN_BRS_OFF;
    tx_header->FDFormat = FDCAN_CLASSIC_CAN;
    tx_header->TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    tx_header->MessageMarker = 0;
}

static void tx_send(FDCAN_HandleTypeDef *hfdcan, uint32_t id, const uint8_t data[8])
{
    FDCAN_TxHeaderTypeDef tx_header;
    HAL_StatusTypeDef status;

    if (!hfdcan || !data) return;

    tx_header_init(&tx_header, id);
    status = HAL_FDCAN_AddMessageToTxFifoQ(hfdcan, &tx_header, (uint8_t *)data);
    (void)status;
}

void can_tx_send_master(FDCAN_HandleTypeDef *hfdcan,
                        const can_state_t *state,
                        motion_profile_t motion_profile)
{
    uint8_t tx_data[8] = {0};
    uint8_t flags;

    if (!hfdcan || !state) return;

    flags = state->night_mode ? MASTER_FLAG_NIGHT : 0u;
    tx_data[0] = PKT_TYPE_MASTER | (flags & 0x0Fu);
    tx_data[1] = state->bank_id;
    tx_data[2] = state->theme_index;
    tx_data[3] = state->oem_color;
    tx_data[4] = (uint8_t)(state->oem_brightness * 5.0f);
    tx_data[5] = (uint8_t)motion_profile;

    tx_send(hfdcan, CAN_MASTER_ID, tx_data);
}

void can_tx_send_sync(FDCAN_HandleTypeDef *hfdcan,
                      uint8_t bank_id,
                      uint8_t theme_index)
{
    uint8_t tx_data[8] = {0};

    if (!hfdcan) return;

    tx_data[0] = PKT_TYPE_SYNC | (bank_id & 0x0Fu);
    tx_data[1] = theme_index;

    tx_send(hfdcan, CAN_MASTER_ID, tx_data);
}

void can_tx_send_discovery(FDCAN_HandleTypeDef *hfdcan,
                           uint32_t unique_id,
                           uint8_t is_master)
{
    uint8_t tx_data[8] = {0};

    if (!hfdcan) return;

    tx_data[0] = PKT_TYPE_DISCOVERY | (BOARD_TYPE & 0x0Fu);
    tx_data[1] = (uint8_t)(unique_id & 0xFFu);
    tx_data[2] = (uint8_t)((unique_id >> 8) & 0xFFu);
    tx_data[3] = (uint8_t)((unique_id >> 16) & 0xFFu);
    tx_data[4] = (uint8_t)((unique_id >> 24) & 0xFFu);
    tx_data[5] = is_master ? 1u : 0u;

    tx_send(hfdcan, CAN_DISCOVERY_ID, tx_data);
}

void can_tx_send_test(FDCAN_HandleTypeDef *hfdcan,
                      uint32_t id,
                      const uint8_t *data,
                      uint8_t len)
{
    uint8_t tx_data[8] = {0};

    if (!hfdcan || !data || len == 0u) return;
    if (len > 8u) len = 8u;

    memcpy(tx_data, data, len);
    tx_send(hfdcan, id, tx_data);
}
