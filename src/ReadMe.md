Edit delay.c, delay.h									Done
Edit systick.c, systich.h								Done
Edit on_board_leds.h, on_board_leds.c 					Done
Edit serial_output.h, serial_output.c					Done
Edit ring_buffer.c, ring_buffer.h						Done
Edit pwm.c, pwm.h										Done
Edit pwm_input.c pwm_input.h							Done
Edit remote_controls.c, remote_controls.h				Done
Edit pid.c, pid.h										Done
Edit i2c.c, i2c.h										Done
Edit accelerometer.c, accelerometer.h					Done
Edit gyroscope.c, gyroscope.h							Done
Edit angular_position.c, angular_position.h				Done
Edit analytics.c, analytics.h							In progress







++++++++++++++++++We have two delay: 	
-One delay software:
	Delay can use 1 second, 1 ms, n ms.
-One delay hardware:
	Delay can use variable second to monitor.

++++++++++++++++++We have function to control led on board
++++++++++++++++++We have function to initialize and send data by usart
++++++++++++++++++We have function ring buffer to manage struct data folow FIFO(First In First Out)