#ifdef __cplusplus
extern "C" {
#endif

static uint32_t masks [] = { 0x7F, // seconds 0-59
                             0x7F, // minutes 0-59
                             0x3F, // hours 0-24
                             0x3F, // day 1-31
                             0x1F, // month 0-12
                             0x07, // day of the week (0 [Sun] - 6 [Sat])
                             0xFF }; // year 0-9999;

extern uint32_t     ds1302rtcRead       (const uint32_t reg) ;
extern void         ds1302rtcWrite      (const uint32_t reg, const uint32_t data) ;

extern uint32_t     ds1302ramRead       (const uint32_t addr) ;
extern void         ds1302ramWrite      (const uint32_t addr, const uint32_t data) ;

extern void         ds1302clockRead     (uint32_t * clockData) ;
extern void         ds1302clockWrite    (const uint32_t * clockData) ;

extern void         ds1302trickleCharge (const uint32_t diodes, const uint32_t resistors) ;

extern void         ds1302setup         (const uint8_t clockPin, const uint8_t dataPin, const uint8_t csPin) ;

extern int32_t      bcdToD              (uint32_t byte, uint32_t mask); 
extern uint32_t     dToBcd              (uint32_t byte);

#ifdef __cplusplus
}
#endif
