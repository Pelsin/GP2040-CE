import { createContext, useCallback, useEffect, useState } from "react";
import * as yup from "yup";

import WebApi from "../Services/WebApi";

type buttonLabels = {
	swapTpShareLabels?: string | boolean;
	buttonLabelType?: string;
};
type AppContextType = {
	buttonLabels: buttonLabels;
	gradientNormalColor1?: string;
	gradientNormalColor2?: string;
	gradientPressedColor1?: string;
	gradientPressedColor2?: string;
	savedColors?: string[];
	usedPins?: never[];
	updateUsedPins: () => Promise<[]>;

	setButtonLabels?: (buttonLabels: buttonLabels) => void;
	setGradientNormalColor1?: (color: string) => void;
	setGradientNormalColor2?: (color: string) => void;
	setGradientPressedColor1?: (color: string) => void;
	setGradientPressedColor2?: (color: string) => void;
	setSavedColors?: (colors: string[] | undefined) => void;
	setUsedPins?: React.Dispatch<React.SetStateAction<never[]>>;
};
export const AppContext = createContext<AppContextType>({
	buttonLabels: { buttonLabelType: "", swapTpShareLabels: "" },
	updateUsedPins: () => Promise.resolve([]),
});

let checkPins = [];

declare module "yup" {
	interface StringSchema {
		validateColor(): this;
	}
	interface NumberSchema {
		validateSelectionWhenValue(
			msg: string,
			value: { label: string; value: number }[]
		): this;
		validateNumberWhenValue(msg: string): this;
		validateMinWhenEqualTo(msg: string, value1: number, value2: number): this;
		validateRangeWhenValue(msg: string, start: number, end: number): this;
		validatePinWhenEqualTo(msg: string, pin: string, value: number): this;
		validatePinWhenValue(msg: string): this;
		checkUsedPins(): this;
	}
}

yup.addMethod(yup.string, "validateColor", function (this: yup.StringSchema) {
	return this.test("", "Valid hex color required", (value) =>
		value?.match(/^#([0-9a-f]{3}|[0-9a-f]{6})$/i)
	);
});

yup.addMethod(
	yup.number,
	"validateSelectionWhenValue",
	function (this: yup.NumberSchema, name, choices) {
		return this.when(name, {
			is: (value: number) => !isNaN(value),
			then: () => this.required().oneOf(choices.map((o) => o.value)),
			otherwise: () => yup.mixed().notRequired(),
		});
	}
);

yup.addMethod(
	yup.number,
	"validateNumberWhenValue",
	function (this: yup.NumberSchema, name) {
		return this.when(name, {
			is: (value: number) => !isNaN(value),
			then: () => this.required(),
			otherwise: () => yup.mixed().notRequired().strip(),
		});
	}
);

yup.addMethod(
	yup.number,
	"validateMinWhenEqualTo",
	function (this: yup.NumberSchema, name, compareValue, min) {
		return this.when(name, {
			is: (value: number) => value === compareValue,
			then: () => this.required().min(min),
			otherwise: () => yup.mixed().notRequired().strip(),
		});
	}
);

yup.addMethod(
	yup.number,
	"validateRangeWhenValue",
	function (this: yup.NumberSchema, name, min, max) {
		return this.when(name, {
			is: (value: number) => !isNaN(value),
			then: () => this.required().min(min).max(max),
			otherwise: () => yup.mixed().notRequired().strip(),
		});
	}
);

yup.addMethod(
	yup.number,
	"validatePinWhenEqualTo",
	function (this: yup.NumberSchema, name, compareName, compareValue) {
		return this.when(compareName, {
			is: (value: number) => value === compareValue,
			then: () => this.validatePinWhenValue(name),
			otherwise: () => yup.mixed().notRequired().strip(),
		});
	}
);

yup.addMethod(
	yup.number,
	"validatePinWhenValue",
	function (this: yup.NumberSchema) {
		return this.checkUsedPins();
	}
);

yup.addMethod(yup.number, "checkUsedPins", function (this: yup.NumberSchema) {
	return this.test(
		"",
		"${originalValue} is unavailable/already assigned!",
		(value) => checkPins(value)
	);
});

export const AppContextProvider = ({
	children,
}: {
	children: React.ReactNode;
}) => {
	const localValue = localStorage.getItem("buttonLabelType") || "gp2040";
	const localValue2 = localStorage.getItem("swapTpShareLabels") || false;
	const [buttonLabels, _setButtonLabels] = useState({
		swapTpShareLabels: localValue2,
		buttonLabelType: localValue,
	});

	const setButtonLabels = useCallback(
		({
			buttonLabelType: newType,
			swapTpShareLabels: newSwap,
		}: buttonLabels) => {
			console.log("buttonLabelType is", newType);
			newType && localStorage.setItem("buttonLabelType", newType);
			newSwap !== undefined &&
				localStorage.setItem("swapTpShareLabels", JSON.stringify(newSwap));
			_setButtonLabels(({ buttonLabelType, swapTpShareLabels }) => ({
				buttonLabelType: newType || buttonLabelType,
				swapTpShareLabels: newSwap !== undefined ? newSwap : swapTpShareLabels,
			}));
		},
		[]
	);

	const [savedColors, _setSavedColors] = useState(
		localStorage.getItem("savedColors")
			? localStorage.getItem("savedColors")?.split(",")
			: []
	);
	const setSavedColors = (savedColors: string[] | undefined) => {
		localStorage.setItem("savedColors", JSON.stringify(savedColors));
		_setSavedColors(savedColors);
	};

	const updateButtonLabels = (e: StorageEvent) => {
		const { key, newValue } = e;
		if (key === "swapTpShareLabels") {
			_setButtonLabels(({ buttonLabelType }) => ({
				buttonLabelType,
				swapTpShareLabels: newValue === "true",
			}));
		}
		if (key === "buttonLabelType" && newValue) {
			_setButtonLabels(({ swapTpShareLabels }) => ({
				buttonLabelType: newValue,
				swapTpShareLabels,
			}));
		}
	};

	useEffect(() => {
		_setButtonLabels({
			buttonLabelType: buttonLabels.buttonLabelType,
			swapTpShareLabels: buttonLabels.swapTpShareLabels,
		});
		window.addEventListener("storage", updateButtonLabels);
		return () => {
			window.removeEventListener("storage", updateButtonLabels);
		};
	}, [buttonLabels.buttonLabelType, buttonLabels.swapTpShareLabels]);

	const [gradientNormalColor1, _setGradientNormalColor1] = useState("#00ffff");
	const setGradientNormalColor1 = (gradientNormalColor1: string) => {
		localStorage.setItem("gradientNormalColor1", gradientNormalColor1);
		_setGradientNormalColor1(gradientNormalColor1);
	};

	const [gradientNormalColor2, _setGradientNormalColor2] = useState("#ff00ff");
	const setGradientNormalColor2 = (gradientNormalColor2: string) => {
		localStorage.setItem("gradientNormalColor2", gradientNormalColor2);
		_setGradientNormalColor2(gradientNormalColor2);
	};

	const [gradientPressedColor1, _setGradientPressedColor1] =
		useState("#ff00ff");
	const setGradientPressedColor1 = (gradientPressedColor1: string) => {
		localStorage.setItem("gradientPressedColor1", gradientPressedColor1);
		_setGradientPressedColor1(gradientPressedColor1);
	};

	const [gradientPressedColor2, _setGradientPressedColor2] =
		useState("#00ffff");
	const setGradientPressedColor2 = (gradientPressedColor2: string) => {
		localStorage.setItem("gradientPressedColor2", gradientPressedColor2);
		_setGradientPressedColor2(gradientPressedColor2);
	};

	const [usedPins, setUsedPins] = useState([]);

	const updateUsedPins = async () => {
		const data = await WebApi.getUsedPins();
		setUsedPins(data.usedPins);
		console.log("usedPins updated:", data.usedPins);
		return data;
	};

	useEffect(() => {
		updateUsedPins();
	}, []);

	useEffect(() => {
		checkPins = (value) => {
			const hasValue = value > -1;
			const isValid =
				value === undefined ||
				value === -1 ||
				(hasValue && value < 30 && (usedPins || []).indexOf(value) === -1);
			return isValid;
		};
	}, [usedPins, setUsedPins]);

	console.log("usedPins:", usedPins);

	return (
		<AppContext.Provider
			value={{
				buttonLabels,
				gradientNormalColor1,
				gradientNormalColor2,
				gradientPressedColor1,
				gradientPressedColor2,
				savedColors,
				usedPins,
				setButtonLabels,
				setGradientNormalColor1,
				setGradientNormalColor2,
				setGradientPressedColor1,
				setGradientPressedColor2,
				setSavedColors,
				setUsedPins,
				updateUsedPins,
			}}
		>
			{children}
		</AppContext.Provider>
	);
};
