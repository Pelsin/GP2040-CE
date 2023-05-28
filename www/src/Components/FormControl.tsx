import { Form } from "react-bootstrap";

type FormControlType = {
	className: string;
	error?: string;
	groupClassName?: string;
	hidden?: boolean;
	label: string;
	labelClassName?: string;
	name: string;
	onBlur?: (event: React.FocusEvent<HTMLInputElement>) => void;
	onClick?: (event: React.MouseEvent<HTMLInputElement>) => void;
	onChange: (event: React.ChangeEvent<HTMLInputElement>) => void;
	type?: string;
	value: number | string;
	isInvalid?: boolean;
	min?: number;
	max?: number;
	maxLength?: number;
};

const FormControl = ({
	onClick,
	label,
	error,
	groupClassName,
	hidden,
	labelClassName,
	...props
}: FormControlType) => {
	return (
		<Form.Group className={groupClassName} onClick={onClick} hidden={hidden}>
			<Form.Label className={labelClassName}>{label}</Form.Label>
			<Form.Control {...props} />
			<Form.Control.Feedback type="invalid">{error}</Form.Control.Feedback>
		</Form.Group>
	);
};

export default FormControl;
