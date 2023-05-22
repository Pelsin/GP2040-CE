import { BrowserRouter, Route, Routes } from "react-router-dom";

import { AppContextProvider } from "./Contexts/AppContext";

import Navigation from "./Components/Navigation";

import HomePage from "./Pages/HomePage";
import SettingsPage from "./Pages/SettingsPage";
import PinMappingPage from "./Pages/PinMapping";
import KeyboardMappingPage from "./Pages/KeyboardMapping";
import ResetSettingsPage from "./Pages/ResetSettingsPage";
import LEDConfigPage from "./Pages/LEDConfigPage";
import CustomThemePage from "./Pages/CustomThemePage";
import DisplayConfigPage from "./Pages/DisplayConfig";
import AddonsConfigPage from "./Pages/AddonsConfigPage";
import BackupPage from "./Pages/BackupPage";
import PlaygroundPage from "./Pages/PlaygroundPage";

import "./App.scss";

const App = () => {
	return (
		<AppContextProvider>
			<BrowserRouter>
				<Navigation />
				<div className="container-fluid body-content">
					<Routes>
						<Route path="/" element={<HomePage />} />
						<Route path="/settings" element={<SettingsPage />} />
						<Route path="/pin-mapping" element={<PinMappingPage />} />
						<Route path="/keyboard-mapping" element={<KeyboardMappingPage />} />
						<Route path="/reset-settings" element={<ResetSettingsPage />} />
						<Route path="/led-config" element={<LEDConfigPage />} />
						<Route path="/custom-theme" element={<CustomThemePage />} />
						<Route path="/display-config" element={<DisplayConfigPage />} />
						<Route path="/add-ons" element={<AddonsConfigPage />} />
						<Route path="/backup" element={<BackupPage />} />
						<Route path="/playground" element={<PlaygroundPage />} />
					</Routes>
				</div>
			</BrowserRouter>
		</AppContextProvider>
	);
};

export default App;
