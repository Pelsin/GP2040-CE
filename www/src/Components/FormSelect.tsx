import { Form, FormSelectProps } from "react-bootstrap";

type FormSelectType = {
	label?: string;
	error?: string;
	groupClassName?: string;
} & FormSelectProps;

const FormSelect = ({
	label,
	error,
	groupClassName,
	...props
}: FormSelectType) => {
	return (
		<Form.Group className={groupClassName}>
			{label && <Form.Label>{label}</Form.Label>}
			<Form.Select {...props} />
			<Form.Control.Feedback type="invalid">{error}</Form.Control.Feedback>
		</Form.Group>
	);
};

export default FormSelect;
