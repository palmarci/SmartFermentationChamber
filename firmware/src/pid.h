#ifndef PID
#define PID

bool pid_control(float setpoint, float temperature, float dt);
void pid_init(float newkp, float newki, float newkd);
float pid_get_kp();
float pid_get_ki();
float pid_get_kd();

#endif /* PID */
