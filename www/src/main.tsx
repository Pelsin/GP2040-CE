import ReactDOM from "react-dom/client";
import App from "./App.tsx";

import "./index.scss";

ReactDOM.createRoot(document.getElementById("root") as HTMLElement).render(
	//  TODO This should be enabled when we correctly fetch async on mount using
	//  react router action or similar instead of useEffect with empty dependency array
	// <React.StrictMode>
	<App />
	// </React.StrictMode>
);
