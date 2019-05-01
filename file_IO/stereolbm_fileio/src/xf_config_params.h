/*
 * SJB: Changed parameters from 11,32,32 to 9,32,4 in order to fit in the 7020 device. I obtained these tested values from table 224 in UG1233
 */

/* SAD window size must be an odd number and it must be less than minimum of image height and width and less than the tested size '21' */
#define SAD_WINDOW_SIZE		9

/* NO_OF_DISPARITIES must be greater than '0' and less than the image width */
#define NO_OF_DISPARITIES	32

/* NO_OF_DISPARITIES must not be lesser than PARALLEL_UNITS and NO_OF_DISPARITIES/PARALLEL_UNITS must be a non-fractional number */
#define PARALLEL_UNITS		4
