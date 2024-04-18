import React, {
	useCallback,
	useContext,
	useEffect,
	useMemo,
	useState,
} from 'react';
import { NavLink } from 'react-router-dom';
import { Alert, Button, Form, Tab, Tabs } from 'react-bootstrap';
import { Trans, useTranslation } from 'react-i18next';
import invert from 'lodash/invert';
import omit from 'lodash/omit';
import zip from 'lodash/zip';

import { AppContext } from '../Contexts/AppContext';
import usePinStore, { MaskPayload, setPinType } from '../Store/usePinStore';
import useProfilesStore from '../Store/useProfilesStore';

import Section from '../Components/Section';
import CustomSelect from '../Components/CustomSelect';
import CaptureButton from '../Components/CaptureButton';

import {
	BUTTON_MASKS,
	BUTTON_MASKS_OPTIONS,
	DPAD_MASKS,
	getButtonLabels,
} from '../Data/Buttons';
import {
	BUTTON_ACTIONS,
	NON_SELECTABLE_BUTTON_ACTIONS,
	PinActionValues,
} from '../Data/Pins';

type PinCell = [string, MaskPayload];
type PinRow = [PinCell, PinCell];
type PinList = [PinRow];

const MASK_OPTIONS = [
	...BUTTON_MASKS.map((mask) => ({ ...mask, type: 'customButtonMask' })),
	...DPAD_MASKS.map((mask) => ({ ...mask, type: 'customDpadMask' })),
];

const getButtonKeyByValue = (value: PinActionValues) =>
	invert(BUTTON_ACTIONS)[value];

const isNonSelectable = (value) =>
	NON_SELECTABLE_BUTTON_ACTIONS.includes(value);

const options = Object.entries(BUTTON_ACTIONS)
	.filter(([, value]) => !isNonSelectable(value))
	.map(([key, value]) => ({
		label: key,
		value,
	}));

const getValue = (pin) => ({
	label: getButtonKeyByValue(pin.action),
	value: pin,
});

type PinsFormTypes = {
	savePins: () => void;
	pins: { [key: string]: MaskPayload };
	setPin: setPinType;
	onCopy?: () => void;
};

const PinsForm = ({ savePins, pins, setPin, onCopy }: PinsFormTypes) => {
	const { buttonLabels, updateUsedPins } = useContext(AppContext);
	const [saveMessage, setSaveMessage] = useState('');

	const { buttonLabelType, swapTpShareLabels } = buttonLabels;
	const CURRENT_BUTTONS = getButtonLabels(buttonLabelType, swapTpShareLabels);
	const buttonNames = omit(CURRENT_BUTTONS, ['label', 'value']);
	const { t } = useTranslation('');

	const handleSubmit = async (e) => {
		e.preventDefault();
		e.stopPropagation();
		try {
			await savePins();
			updateUsedPins();
			setSaveMessage(t('Common:saved-success-message'));
		} catch (error) {
			setSaveMessage(t('Common:saved-error-message'));
		}
	};

	const pinList = useMemo<PinList>(() => {
		const pinArray = Object.entries(pins);
		return zip(
			pinArray.slice(0, pinArray.length / 2),
			pinArray.slice(pinArray.length / 2, pinArray.length),
		);
	}, [pins]);

	const createCell = useCallback(
		([pin, pinData]: PinCell) => (
			<div className="d-flex col py-2">
				<div className="d-flex align-items-center" style={{ width: '4rem' }}>
					<label htmlFor={pin}>{pin.toUpperCase()}</label>
				</div>
				<CustomSelect
					inputId={pin}
					isClearable
					isSearchable
					options={options}
					value={getValue(pinData)}
					isDisabled={isNonSelectable(pinData.action)}
					getOptionLabel={(option) => {
						const labelKey = option.label.split('BUTTON_PRESS_').pop();
						// Need to fallback as some button actions are not part of button names
						return (
							(labelKey && buttonNames[labelKey]) ||
							t(`PinMapping:actions.${option.label}`)
						);
					}}
					// onChange={(change) =>
					// 	setPin(
					// 		pin,
					// 		change?.value === undefined ? -10 : change.value, // On clear set to -10
					// 	)
					// }
					isMulti={
						pinData.action === BUTTON_ACTIONS.CUSTOM_BUTTON_COMBO ||
						MASK_OPTIONS.some(
							({ label }) =>
								label.toUpperCase() ===
								getButtonKeyByValue(pinData.action)
									.split('BUTTON_PRESS_')
									.pop(),
						)
					}
					onChange={
						(selected) => {
							console.log('selected', selected);
						}
						// setPin(
						// 	pin,
						// 	selected.reduce(
						// 		(masks, option) => ({
						// 			customButtonMask:
						// 				option.type === 'customButtonMask'
						// 					? masks.customButtonMask ^ option.value
						// 					: masks.customButtonMask,
						// 			customDpadMask:
						// 				option.type === 'customDpadMask'
						// 					? masks.customDpadMask ^ option.value
						// 					: masks.customDpadMask,
						// 		}),
						// 		{ customButtonMask: 0, customDpadMask: 0 },
						// 	),
						// )
					}
				/>
			</div>
		),
		[buttonNames],
	);

	return (
		<Form onSubmit={handleSubmit}>
			<div className="d-flex">
				<div className="py-3">
					<CaptureButton
						labels={Object.values(buttonNames)}
						onChange={(label, pin) =>
							setPin(
								// Convert getHeldPins format to setPinMappings format
								parseInt(pin) < 10 ? `pin0${pin}` : `pin${pin}`,
								// Maps current mode buttons to actions
								BUTTON_ACTIONS[
									`BUTTON_PRESS_${invert(buttonNames)[label].toUpperCase()}`
								],
							)
						}
					/>
				</div>
				{onCopy && (
					<div className="py-3 mx-3">
						<Button variant="secondary" onClick={() => onCopy()}>
							{t(`PinMapping:profile-copy-base`)}
						</Button>
					</div>
				)}
			</div>
			<div className="gx-3">
				{pinList.map(([cell1, cell2], i) => (
					<div key={`pin-row-${i}`} className="row">
						{createCell(cell1)}
						{createCell(cell2)}
					</div>
				))}
			</div>
			<Button type="submit" className="my-4">
				{t('Common:button-save-label')}
			</Button>
			{saveMessage && <Alert variant="secondary">{saveMessage}</Alert>}
		</Form>
	);
};

export default function PinMappingPage() {
	const { fetchPins, pins, savePins, setPin } = usePinStore();
	const {
		fetchProfiles,
		profiles,
		saveProfiles,
		setProfileAction,
		setProfile,
	} = useProfilesStore();

	const [pressedPin, setPressedPin] = useState<number | null>(null);

	const { t } = useTranslation('');

	useEffect(() => {
		fetchPins();
		fetchProfiles();
	}, []);

	const saveAll = useCallback(() => {
		savePins();
		saveProfiles();
	}, [savePins, saveProfiles]);

	return (
		<>
			<div className="alert alert-warning">
				<Trans ns="PinMapping" i18nKey="alert-text">
					Mapping buttons to pins that aren&apos;t connected or available can
					leave the device in non-functional state. To clear the invalid
					configuration go to the{' '}
					<NavLink to="/reset-settings">Reset Settings</NavLink> page.
				</Trans>
				<br />
				<br />
				{t(`PinMapping:profile-pins-warning`)}
			</div>
			<Section title={t('PinMapping:header-text')}>
				<p>{t('PinMapping:sub-header-text')}</p>
				<div className="mb-3">
					<CaptureButton
						buttonLabel={t('PinMapping:pin-viewer')}
						labels={['']}
						onChange={(_, pin) => setPressedPin(pin)}
					/>
				</div>
				{pressedPin !== null && (
					<div className="alert alert-info">
						<strong>{t('PinMapping:pin-pressed', { pressedPin })}</strong>
					</div>
				)}

				<Tabs id="profiles">
					<Tab eventKey="Base" title="Base(Profile 1)">
						<PinsForm pins={pins} savePins={saveAll} setPin={setPin} />
					</Tab>
					{/* {profiles.map((profilePins, profileIndex) => (
						<Tab
							key={`Profile${profileIndex + 2}`}
							eventKey={`Profile${profileIndex + 2}`}
							title={`Profile ${profileIndex + 2}`}
						>
							<PinsForm
								pins={profilePins}
								savePins={saveAll}
								setPin={(pin, action) => {
									setProfileAction(profileIndex, pin, action);
								}}
								onCopy={() => {
									setProfile(profileIndex, pins);
								}}
							/>
						</Tab>
					))} */}
				</Tabs>
			</Section>
		</>
	);
}
