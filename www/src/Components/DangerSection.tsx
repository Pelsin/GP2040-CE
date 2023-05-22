import Section from "./Section";

const DangerSection = ({
	title,
	children,
}: {
	title: string;
	children: React.ReactNode;
}) => {
	return (
		<Section
			className={`border-danger`}
			titleClassName={`text-white bg-danger`}
			title={title}
		>
			{children}
		</Section>
	);
};

export default DangerSection;
