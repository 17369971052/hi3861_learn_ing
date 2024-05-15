#ifndef SG92R_CONTROL_H
#define SG92R_CONTROL_H

typedef enum
{
	Control_OFF = 0,
	Control_ON
} Servo_Status_ENUM;

void SG92RInit(void);
void Turn_Where(Servo_Status_ENUM status);


#endif /* __SG92R_CONTROL_H__ */
