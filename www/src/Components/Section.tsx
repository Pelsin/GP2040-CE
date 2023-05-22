import "./Section.scss";

type SectionProps = {
	title: string;
	className?: string;
	titleClassName?: string;
	children: React.ReactNode;
};

const Section = ({
	children,
	title,
	className,
	titleClassName,
}: SectionProps) => {
	return (
		<div className={`card ${className}`}>
			<div className={`card-header ${titleClassName}`}>
				<strong>{title}</strong>
			</div>
			<div className="card-body">{children}</div>
		</div>
	);
};

export default Section;
