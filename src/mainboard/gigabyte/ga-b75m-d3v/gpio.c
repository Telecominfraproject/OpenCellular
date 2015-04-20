#include "southbridge/intel/bd82x6x/gpio.h"
static const struct pch_gpio_set1 pch_gpio_set1_mode = {
	.gpio0 = GPIO_MODE_NATIVE,
	.gpio1 = GPIO_MODE_NATIVE,
	.gpio2 = GPIO_MODE_NATIVE,
	.gpio3 = GPIO_MODE_NATIVE,
	.gpio4 = GPIO_MODE_NATIVE,
	.gpio5 = GPIO_MODE_NATIVE,
	.gpio6 = GPIO_MODE_NATIVE,
	.gpio7 = GPIO_MODE_NATIVE,
	.gpio8 = GPIO_MODE_NATIVE,
	.gpio9 = GPIO_MODE_NATIVE,
	.gpio10 = GPIO_MODE_NATIVE,
	.gpio11 = GPIO_MODE_NATIVE,
	.gpio12 = GPIO_MODE_GPIO,
	.gpio13 = GPIO_MODE_NATIVE,
	.gpio14 = GPIO_MODE_NATIVE,
	.gpio15 = GPIO_MODE_NATIVE,
	.gpio16 = GPIO_MODE_NATIVE,
	.gpio17 = GPIO_MODE_NATIVE,
	.gpio18 = GPIO_MODE_NATIVE,
	.gpio19 = GPIO_MODE_NATIVE,
	.gpio20 = GPIO_MODE_NATIVE,
	.gpio21 = GPIO_MODE_NATIVE,
	.gpio22 = GPIO_MODE_NATIVE,
	.gpio23 = GPIO_MODE_NATIVE,
	.gpio24 = GPIO_MODE_NATIVE,
	.gpio25 = GPIO_MODE_NATIVE,
	.gpio26 = GPIO_MODE_NATIVE,
	.gpio27 = GPIO_MODE_NATIVE,
	.gpio28 = GPIO_MODE_NATIVE,
	.gpio29 = GPIO_MODE_NATIVE,
	.gpio30 = GPIO_MODE_NATIVE,
	.gpio31 = GPIO_MODE_NATIVE,
};

static const struct pch_gpio_set1 pch_gpio_set1_direction = {
	.gpio0 = GPIO_DIR_OUTPUT,
	.gpio1 = GPIO_DIR_OUTPUT,
	.gpio2 = GPIO_DIR_OUTPUT,
	.gpio3 = GPIO_DIR_OUTPUT,
	.gpio4 = GPIO_DIR_OUTPUT,
	.gpio5 = GPIO_DIR_OUTPUT,
	.gpio6 = GPIO_DIR_OUTPUT,
	.gpio7 = GPIO_DIR_OUTPUT,
	.gpio8 = GPIO_DIR_OUTPUT,
	.gpio9 = GPIO_DIR_OUTPUT,
	.gpio10 = GPIO_DIR_OUTPUT,
	.gpio11 = GPIO_DIR_OUTPUT,
	.gpio12 = GPIO_DIR_OUTPUT,
	.gpio13 = GPIO_DIR_OUTPUT,
	.gpio14 = GPIO_DIR_OUTPUT,
	.gpio15 = GPIO_DIR_OUTPUT,
	.gpio16 = GPIO_DIR_OUTPUT,
	.gpio17 = GPIO_DIR_OUTPUT,
	.gpio18 = GPIO_DIR_OUTPUT,
	.gpio19 = GPIO_DIR_OUTPUT,
	.gpio20 = GPIO_DIR_OUTPUT,
	.gpio21 = GPIO_DIR_OUTPUT,
	.gpio22 = GPIO_DIR_OUTPUT,
	.gpio23 = GPIO_DIR_OUTPUT,
	.gpio24 = GPIO_DIR_OUTPUT,
	.gpio25 = GPIO_DIR_OUTPUT,
	.gpio26 = GPIO_DIR_OUTPUT,
	.gpio27 = GPIO_DIR_OUTPUT,
	.gpio28 = GPIO_DIR_OUTPUT,
	.gpio29 = GPIO_DIR_OUTPUT,
	.gpio30 = GPIO_DIR_INPUT,
	.gpio31 = GPIO_DIR_OUTPUT,
};

static const struct pch_gpio_set1 pch_gpio_set1_level = {
	.gpio0 = GPIO_LEVEL_HIGH,
	.gpio1 = GPIO_LEVEL_HIGH,
	.gpio2 = GPIO_LEVEL_HIGH,
	.gpio3 = GPIO_LEVEL_HIGH,
	.gpio4 = GPIO_LEVEL_HIGH,
	.gpio5 = GPIO_LEVEL_HIGH,
	.gpio6 = GPIO_LEVEL_HIGH,
	.gpio7 = GPIO_LEVEL_HIGH,
	.gpio8 = GPIO_LEVEL_LOW,
	.gpio9 = GPIO_LEVEL_HIGH,
	.gpio10 = GPIO_LEVEL_HIGH,
	.gpio11 = GPIO_LEVEL_HIGH,
	.gpio12 = GPIO_LEVEL_HIGH,
	.gpio13 = GPIO_LEVEL_HIGH,
	.gpio14 = GPIO_LEVEL_HIGH,
	.gpio15 = GPIO_LEVEL_LOW,
	.gpio16 = GPIO_LEVEL_HIGH,
	.gpio17 = GPIO_LEVEL_LOW,
	.gpio18 = GPIO_LEVEL_HIGH,
	.gpio19 = GPIO_LEVEL_LOW,
	.gpio20 = GPIO_LEVEL_LOW,
	.gpio21 = GPIO_LEVEL_LOW,
	.gpio22 = GPIO_LEVEL_LOW,
	.gpio23 = GPIO_LEVEL_LOW,
	.gpio24 = GPIO_LEVEL_LOW,
	.gpio25 = GPIO_LEVEL_HIGH,
	.gpio26 = GPIO_LEVEL_LOW,
	.gpio27 = GPIO_LEVEL_HIGH,
	.gpio28 = GPIO_LEVEL_LOW,
	.gpio29 = GPIO_LEVEL_HIGH,
	.gpio30 = GPIO_LEVEL_HIGH,
	.gpio31 = GPIO_LEVEL_HIGH,
};

static const struct pch_gpio_set1 pch_gpio_set1_reset = {
	.gpio0 = GPIO_RESET_PWROK,
	.gpio1 = GPIO_RESET_PWROK,
	.gpio2 = GPIO_RESET_PWROK,
	.gpio3 = GPIO_RESET_PWROK,
	.gpio4 = GPIO_RESET_PWROK,
	.gpio5 = GPIO_RESET_PWROK,
	.gpio6 = GPIO_RESET_PWROK,
	.gpio7 = GPIO_RESET_PWROK,
	.gpio8 = GPIO_RESET_PWROK,
	.gpio9 = GPIO_RESET_PWROK,
	.gpio10 = GPIO_RESET_PWROK,
	.gpio11 = GPIO_RESET_PWROK,
	.gpio12 = GPIO_RESET_PWROK,
	.gpio13 = GPIO_RESET_PWROK,
	.gpio14 = GPIO_RESET_PWROK,
	.gpio15 = GPIO_RESET_PWROK,
	.gpio16 = GPIO_RESET_PWROK,
	.gpio17 = GPIO_RESET_PWROK,
	.gpio18 = GPIO_RESET_PWROK,
	.gpio19 = GPIO_RESET_PWROK,
	.gpio20 = GPIO_RESET_PWROK,
	.gpio21 = GPIO_RESET_PWROK,
	.gpio22 = GPIO_RESET_PWROK,
	.gpio23 = GPIO_RESET_PWROK,
	.gpio24 = GPIO_RESET_RSMRST,
	.gpio25 = GPIO_RESET_PWROK,
	.gpio26 = GPIO_RESET_PWROK,
	.gpio27 = GPIO_RESET_PWROK,
	.gpio28 = GPIO_RESET_PWROK,
	.gpio29 = GPIO_RESET_PWROK,
	.gpio30 = GPIO_RESET_PWROK,
	.gpio31 = GPIO_RESET_PWROK,
};

static const struct pch_gpio_set1 pch_gpio_set1_invert = {
	.gpio0 = GPIO_NO_INVERT,
	.gpio1 = GPIO_NO_INVERT,
	.gpio2 = GPIO_NO_INVERT,
	.gpio3 = GPIO_NO_INVERT,
	.gpio4 = GPIO_NO_INVERT,
	.gpio5 = GPIO_NO_INVERT,
	.gpio6 = GPIO_NO_INVERT,
	.gpio7 = GPIO_NO_INVERT,
	.gpio8 = GPIO_NO_INVERT,
	.gpio9 = GPIO_NO_INVERT,
	.gpio10 = GPIO_NO_INVERT,
	.gpio11 = GPIO_NO_INVERT,
	.gpio12 = GPIO_NO_INVERT,
	.gpio13 = GPIO_INVERT,
	.gpio14 = GPIO_NO_INVERT,
	.gpio15 = GPIO_NO_INVERT,
	.gpio16 = GPIO_NO_INVERT,
	.gpio17 = GPIO_NO_INVERT,
	.gpio18 = GPIO_NO_INVERT,
	.gpio19 = GPIO_NO_INVERT,
	.gpio20 = GPIO_NO_INVERT,
	.gpio21 = GPIO_NO_INVERT,
	.gpio22 = GPIO_NO_INVERT,
	.gpio23 = GPIO_NO_INVERT,
	.gpio24 = GPIO_NO_INVERT,
	.gpio25 = GPIO_NO_INVERT,
	.gpio26 = GPIO_NO_INVERT,
	.gpio27 = GPIO_NO_INVERT,
	.gpio28 = GPIO_NO_INVERT,
	.gpio29 = GPIO_NO_INVERT,
	.gpio30 = GPIO_NO_INVERT,
	.gpio31 = GPIO_NO_INVERT,
};

static const struct pch_gpio_set1 pch_gpio_set1_blink = {
	.gpio0 = GPIO_NO_BLINK,
	.gpio1 = GPIO_NO_BLINK,
	.gpio2 = GPIO_NO_BLINK,
	.gpio3 = GPIO_NO_BLINK,
	.gpio4 = GPIO_NO_BLINK,
	.gpio5 = GPIO_NO_BLINK,
	.gpio6 = GPIO_NO_BLINK,
	.gpio7 = GPIO_NO_BLINK,
	.gpio8 = GPIO_NO_BLINK,
	.gpio9 = GPIO_NO_BLINK,
	.gpio10 = GPIO_NO_BLINK,
	.gpio11 = GPIO_NO_BLINK,
	.gpio12 = GPIO_NO_BLINK,
	.gpio13 = GPIO_NO_BLINK,
	.gpio14 = GPIO_NO_BLINK,
	.gpio15 = GPIO_NO_BLINK,
	.gpio16 = GPIO_NO_BLINK,
	.gpio17 = GPIO_NO_BLINK,
	.gpio18 = GPIO_BLINK,
	.gpio19 = GPIO_NO_BLINK,
	.gpio20 = GPIO_NO_BLINK,
	.gpio21 = GPIO_NO_BLINK,
	.gpio22 = GPIO_NO_BLINK,
	.gpio23 = GPIO_NO_BLINK,
	.gpio24 = GPIO_NO_BLINK,
	.gpio25 = GPIO_NO_BLINK,
	.gpio26 = GPIO_NO_BLINK,
	.gpio27 = GPIO_NO_BLINK,
	.gpio28 = GPIO_NO_BLINK,
	.gpio29 = GPIO_NO_BLINK,
	.gpio30 = GPIO_NO_BLINK,
	.gpio31 = GPIO_NO_BLINK,
};

static const struct pch_gpio_set2 pch_gpio_set2_mode = {
	.gpio32 = GPIO_MODE_GPIO,
	.gpio33 = GPIO_MODE_GPIO,
	.gpio34 = GPIO_MODE_GPIO,
	.gpio35 = GPIO_MODE_GPIO,
	.gpio36 = GPIO_MODE_GPIO,
	.gpio37 = GPIO_MODE_GPIO,
	.gpio38 = GPIO_MODE_GPIO,
	.gpio39 = GPIO_MODE_GPIO,
	.gpio40 = GPIO_MODE_NATIVE,
	.gpio41 = GPIO_MODE_NATIVE,
	.gpio42 = GPIO_MODE_NATIVE,
	.gpio43 = GPIO_MODE_NATIVE,
	.gpio44 = GPIO_MODE_NATIVE,
	.gpio45 = GPIO_MODE_NATIVE,
	.gpio46 = GPIO_MODE_NATIVE,
	.gpio47 = GPIO_MODE_NATIVE,
	.gpio48 = GPIO_MODE_GPIO,
	.gpio49 = GPIO_MODE_GPIO,
	.gpio50 = GPIO_MODE_NATIVE,
	.gpio51 = GPIO_MODE_NATIVE,
	.gpio52 = GPIO_MODE_NATIVE,
	.gpio53 = GPIO_MODE_NATIVE,
	.gpio54 = GPIO_MODE_NATIVE,
	.gpio55 = GPIO_MODE_NATIVE,
	.gpio56 = GPIO_MODE_NATIVE,
	.gpio57 = GPIO_MODE_GPIO,
	.gpio58 = GPIO_MODE_NATIVE,
	.gpio59 = GPIO_MODE_NATIVE,
	.gpio60 = GPIO_MODE_NATIVE,
	.gpio61 = GPIO_MODE_NATIVE,
	.gpio62 = GPIO_MODE_NATIVE,
	.gpio63 = GPIO_MODE_NATIVE,
};

static const struct pch_gpio_set2 pch_gpio_set2_direction = {
	.gpio32 = GPIO_DIR_OUTPUT,
	.gpio33 = GPIO_DIR_OUTPUT,
	.gpio34 = GPIO_DIR_INPUT,
	.gpio35 = GPIO_DIR_OUTPUT,
	.gpio36 = GPIO_DIR_INPUT,
	.gpio37 = GPIO_DIR_INPUT,
	.gpio38 = GPIO_DIR_INPUT,
	.gpio39 = GPIO_DIR_INPUT,
	.gpio40 = GPIO_DIR_INPUT,
	.gpio41 = GPIO_DIR_INPUT,
	.gpio42 = GPIO_DIR_INPUT,
	.gpio43 = GPIO_DIR_INPUT,
	.gpio44 = GPIO_DIR_INPUT,
	.gpio45 = GPIO_DIR_INPUT,
	.gpio46 = GPIO_DIR_INPUT,
	.gpio47 = GPIO_DIR_INPUT,
	.gpio48 = GPIO_DIR_INPUT,
	.gpio49 = GPIO_DIR_INPUT,
	.gpio50 = GPIO_DIR_INPUT,
	.gpio51 = GPIO_DIR_OUTPUT,
	.gpio52 = GPIO_DIR_INPUT,
	.gpio53 = GPIO_DIR_OUTPUT,
	.gpio54 = GPIO_DIR_INPUT,
	.gpio55 = GPIO_DIR_OUTPUT,
	.gpio56 = GPIO_DIR_INPUT,
	.gpio57 = GPIO_DIR_INPUT,
	.gpio58 = GPIO_DIR_INPUT,
	.gpio59 = GPIO_DIR_INPUT,
	.gpio60 = GPIO_DIR_INPUT,
	.gpio61 = GPIO_DIR_OUTPUT,
	.gpio62 = GPIO_DIR_OUTPUT,
	.gpio63 = GPIO_DIR_OUTPUT,
};

static const struct pch_gpio_set2 pch_gpio_set2_level = {
	.gpio32 = GPIO_LEVEL_LOW,
	.gpio33 = GPIO_LEVEL_LOW,
	.gpio34 = GPIO_LEVEL_LOW,
	.gpio35 = GPIO_LEVEL_LOW,
	.gpio36 = GPIO_LEVEL_LOW,
	.gpio37 = GPIO_LEVEL_LOW,
	.gpio38 = GPIO_LEVEL_HIGH,
	.gpio39 = GPIO_LEVEL_HIGH,
	.gpio40 = GPIO_LEVEL_HIGH,
	.gpio41 = GPIO_LEVEL_HIGH,
	.gpio42 = GPIO_LEVEL_HIGH,
	.gpio43 = GPIO_LEVEL_HIGH,
	.gpio44 = GPIO_LEVEL_HIGH,
	.gpio45 = GPIO_LEVEL_HIGH,
	.gpio46 = GPIO_LEVEL_HIGH,
	.gpio47 = GPIO_LEVEL_LOW,
	.gpio48 = GPIO_LEVEL_HIGH,
	.gpio49 = GPIO_LEVEL_LOW,
	.gpio50 = GPIO_LEVEL_HIGH,
	.gpio51 = GPIO_LEVEL_LOW,
	.gpio52 = GPIO_LEVEL_HIGH,
	.gpio53 = GPIO_LEVEL_LOW,
	.gpio54 = GPIO_LEVEL_HIGH,
	.gpio55 = GPIO_LEVEL_LOW,
	.gpio56 = GPIO_LEVEL_LOW,
	.gpio57 = GPIO_LEVEL_HIGH,
	.gpio58 = GPIO_LEVEL_HIGH,
	.gpio59 = GPIO_LEVEL_HIGH,
	.gpio60 = GPIO_LEVEL_HIGH,
	.gpio61 = GPIO_LEVEL_LOW,
	.gpio62 = GPIO_LEVEL_HIGH,
	.gpio63 = GPIO_LEVEL_LOW,
};

static const struct pch_gpio_set2 pch_gpio_set2_reset = {
	.gpio32 = GPIO_RESET_PWROK,
	.gpio33 = GPIO_RESET_PWROK,
	.gpio34 = GPIO_RESET_PWROK,
	.gpio35 = GPIO_RESET_PWROK,
	.gpio36 = GPIO_RESET_PWROK,
	.gpio37 = GPIO_RESET_PWROK,
	.gpio38 = GPIO_RESET_PWROK,
	.gpio39 = GPIO_RESET_PWROK,
	.gpio40 = GPIO_RESET_PWROK,
	.gpio41 = GPIO_RESET_PWROK,
	.gpio42 = GPIO_RESET_PWROK,
	.gpio43 = GPIO_RESET_PWROK,
	.gpio44 = GPIO_RESET_PWROK,
	.gpio45 = GPIO_RESET_PWROK,
	.gpio46 = GPIO_RESET_PWROK,
	.gpio47 = GPIO_RESET_PWROK,
	.gpio48 = GPIO_RESET_PWROK,
	.gpio49 = GPIO_RESET_PWROK,
	.gpio50 = GPIO_RESET_PWROK,
	.gpio51 = GPIO_RESET_PWROK,
	.gpio52 = GPIO_RESET_PWROK,
	.gpio53 = GPIO_RESET_PWROK,
	.gpio54 = GPIO_RESET_PWROK,
	.gpio55 = GPIO_RESET_PWROK,
	.gpio56 = GPIO_RESET_PWROK,
	.gpio57 = GPIO_RESET_PWROK,
	.gpio58 = GPIO_RESET_PWROK,
	.gpio59 = GPIO_RESET_PWROK,
	.gpio60 = GPIO_RESET_PWROK,
	.gpio61 = GPIO_RESET_PWROK,
	.gpio62 = GPIO_RESET_PWROK,
	.gpio63 = GPIO_RESET_PWROK,
};

static const struct pch_gpio_set3 pch_gpio_set3_mode = {
	.gpio64 = GPIO_MODE_NATIVE,
	.gpio65 = GPIO_MODE_NATIVE,
	.gpio66 = GPIO_MODE_NATIVE,
	.gpio67 = GPIO_MODE_NATIVE,
	.gpio68 = GPIO_MODE_GPIO,
	.gpio69 = GPIO_MODE_GPIO,
	.gpio70 = GPIO_MODE_NATIVE,
	.gpio71 = GPIO_MODE_NATIVE,
	.gpio72 = GPIO_MODE_GPIO,
	.gpio73 = GPIO_MODE_NATIVE,
	.gpio74 = GPIO_MODE_NATIVE,
	.gpio75 = GPIO_MODE_NATIVE,
};

static const struct pch_gpio_set3 pch_gpio_set3_direction = {
	.gpio64 = GPIO_DIR_OUTPUT,
	.gpio65 = GPIO_DIR_OUTPUT,
	.gpio66 = GPIO_DIR_OUTPUT,
	.gpio67 = GPIO_DIR_OUTPUT,
	.gpio68 = GPIO_DIR_INPUT,
	.gpio69 = GPIO_DIR_INPUT,
	.gpio70 = GPIO_DIR_INPUT,
	.gpio71 = GPIO_DIR_INPUT,
	.gpio72 = GPIO_DIR_INPUT,
	.gpio73 = GPIO_DIR_INPUT,
	.gpio74 = GPIO_DIR_INPUT,
	.gpio75 = GPIO_DIR_INPUT,
};

static const struct pch_gpio_set3 pch_gpio_set3_level = {
	.gpio64 = GPIO_LEVEL_HIGH,
	.gpio65 = GPIO_LEVEL_HIGH,
	.gpio66 = GPIO_LEVEL_HIGH,
	.gpio67 = GPIO_LEVEL_HIGH,
	.gpio68 = GPIO_LEVEL_HIGH,
	.gpio69 = GPIO_LEVEL_LOW,
	.gpio70 = GPIO_LEVEL_LOW,
	.gpio71 = GPIO_LEVEL_LOW,
	.gpio72 = GPIO_LEVEL_HIGH,
	.gpio73 = GPIO_LEVEL_LOW,
	.gpio74 = GPIO_LEVEL_HIGH,
	.gpio75 = GPIO_LEVEL_HIGH,
};

static const struct pch_gpio_set3 pch_gpio_set3_reset = {
	.gpio64 = GPIO_RESET_PWROK,
	.gpio65 = GPIO_RESET_PWROK,
	.gpio66 = GPIO_RESET_PWROK,
	.gpio67 = GPIO_RESET_PWROK,
	.gpio68 = GPIO_RESET_PWROK,
	.gpio69 = GPIO_RESET_PWROK,
	.gpio70 = GPIO_RESET_PWROK,
	.gpio71 = GPIO_RESET_PWROK,
	.gpio72 = GPIO_RESET_PWROK,
	.gpio73 = GPIO_RESET_PWROK,
	.gpio74 = GPIO_RESET_PWROK,
	.gpio75 = GPIO_RESET_PWROK,
};

const struct pch_gpio_map mainboard_gpio_map = {
	.set1 = {
		.mode		= &pch_gpio_set1_mode,
		.direction	= &pch_gpio_set1_direction,
		.level		= &pch_gpio_set1_level,
		.blink		= &pch_gpio_set1_blink,
		.invert		= &pch_gpio_set1_invert,
		.reset		= &pch_gpio_set1_reset,
	},
	.set2 = {
		.mode		= &pch_gpio_set2_mode,
		.direction	= &pch_gpio_set2_direction,
		.level		= &pch_gpio_set2_level,
		.reset		= &pch_gpio_set2_reset,
	},
	.set3 = {
		.mode		= &pch_gpio_set3_mode,
		.direction	= &pch_gpio_set3_direction,
		.level		= &pch_gpio_set3_level,
		.reset		= &pch_gpio_set3_reset,
	},
};
