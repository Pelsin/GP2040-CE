export const displayOptions = {
	enabled: 0,
	sdaPin: -1,
	sclPin: -1,
	i2cAddress: "0x3C",
	i2cBlock: 0,
	i2cSpeed: 400000,
	flipDisplay: 0,
	invertDisplay: 0,
	buttonLayout: 0,
	buttonLayoutRight: 3,
	splashDuration: 0,
	splashMode: 3,
	splashImage: Array(16 * 64).fill(0), // 128 columns represented by bytes so 16 and 64 rows
	invertSplash: false,
	buttonLayoutCustomOptions: {
		params: {
			layout: 0,
			startX: 8,
			startY: 28,
			buttonRadius: 8,
			buttonPadding: 2,
		},
		paramsRight: {
			layout: 3,
			startX: 8,
			startY: 28,
			buttonRadius: 8,
			buttonPadding: 2,
		},
	},
	displaySaverTimeout: 0,
};

export type displayOptionsType = typeof displayOptions;
