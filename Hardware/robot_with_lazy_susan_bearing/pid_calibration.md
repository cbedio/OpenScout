# Speed PID controller calibration

## Calibration time = 20 minutes

The robot drives using [differential robot kinematics](https://www.cs.columbia.edu/~allen/F19/NOTES/icckinematics.pdf). We use PID controller to set the desired speed (linear and angular) which is then converted in rad/s. To tune the PID controller, all three gain values are initially set to zero and then tuned independently by comparing the actual and the desired motor speed in the Arduino IDE's serial plotter (To open the serial plotter in the IDE `Tools -> Serial Plotter`).

1. Firstly, the motors’ speed is set in a square wave between 1/3 and 2/3 of the maximum speed (in rad/s). The maximum speed does not need to be found, rather than getting a rough estimation of it. The rationale behind the `[1/3,2/3]` speed interval is to produce a speed flunctuation, but the actual values can be arbitrary. 
2. `Kp` is gradually increased until motors reach the desired speed in the Serial Plotter. Notice that the actual speed will never reach the set speed. A good `Kp` good value is chosen when the actual speed is slightly less than the desired speed. 
3. Next, `Ki` is gradually increased so that the motors speed converges to the desired speed. Oscillations around the setpoint will occur from overshooting. At this point, the motors can reach and oscillate around the desired speed. Increase the $Ki$ until you observe a slight oscillation around the desired speed.
4. To reduce overshoooting, gradually increase `Kd` until the oscillations around the setpoint are removed.

In our implemenation we found `Kp=2, Ki=5, Kd=1` as optimal.

