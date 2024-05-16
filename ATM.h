#ifndef ATM_H_
#define ATM_H_

enum ATM_states {
    CREDIT_CARD_SALDO,
    PIN_CODE,
    CHECK_PIN,
    WITHDRAWAL_AMOUNTS,
    WITHDRAWAL_OPTIONS_PRINT
};

void ATM_task(void *pvParameters);
void set_ATM_state(enum ATM_states new_state);

#endif /* ATM_H_ */
