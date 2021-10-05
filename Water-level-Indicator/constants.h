/**
 * @file constants.h
 * @author Arkadip Bhatacharya (in2arkadipb13@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2021-10-05
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#define DISPLAY_I2C 1   // Display for I2c holds value 1
#define DISPLAY_SEEED 2 // Display for seeed holds value 2


#define SERIAL_DEBUG 1 //Mode for serial debug, 0 -> No debug, 1 -> Debug over serial is on

// Constants for distances
#define SPEED_OF_SOUND 0.034 // speed of sound(cm/us)
#define ITER_M 10 // Interation for median checking

// Tank dimention
#define TANK_BUTTOM_DISTANCE 104    // Distance of the water lavel, when the tank is empty
#define TANK_TOP_DISTANCE 25    // Distance of the water lavel, when the tank is full
#define TANK_REDIUS 50

#define BUZZER_THRESHOLD_PERCENTAGE 95      // The threshold percentage, after which the buzzer blows up
#define STOP_THRESHOLD_PERCENTAGE 99        // The threshold percentage, after which the self stop commend turns on