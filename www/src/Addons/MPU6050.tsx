import { useContext } from 'react';
import { Trans, useTranslation } from 'react-i18next';
import { FormCheck, FormLabel } from 'react-bootstrap';
import { NavLink } from 'react-router-dom';
import * as yup from 'yup';

import Section from '../Components/Section';
import { AppContext } from '../Contexts/AppContext';
import { AddonPropTypes } from '../Pages/AddonsConfigPage';

export const mpu6050Scheme = {
	MPU6050InputEnabled: yup.number().label('MPU6050 Input Enabled'),
	mpu6050AccelEnabled: yup.number().label('MPU6050 Accelerometer Enabled'),
	mpu6050GyroEnabled: yup.number().label('MPU6050 Gyroscope Enabled'),
};

export const mpu6050State = {
	MPU6050InputEnabled: 0,
	mpu6050AccelEnabled: 1,
	mpu6050GyroEnabled: 0,
};

const MPU6050 = ({ values, errors, handleChange, handleCheckbox }: AddonPropTypes) => {
	const { t } = useTranslation();
	const { getAvailablePeripherals } = useContext(AppContext);

	return (
		<Section title={t('AddonsConfig:mpu6050-header-text')}>
			<div
				id="MPU6050InputOptions"
				hidden={!(values.MPU6050InputEnabled && getAvailablePeripherals('i2c'))}
			>
				<div className="alert alert-info" role="alert">
					{t('AddonsConfig:mpu6050-peripheral-info')}{' '}
					<a href="../peripheral-mapping" className="alert-link">
						{t('AddonsConfig:mpu6050-peripheral-link')}
					</a>
					.
				</div>
				<FormCheck
					label={t('AddonsConfig:mpu6050-accel-enabled-label')}
					type="switch"
					id="mpu6050AccelEnabledButton"
					reverse
					isInvalid={false}
					checked={Boolean(values.mpu6050AccelEnabled)}
					onChange={(e) => {
						handleCheckbox('mpu6050AccelEnabled');
						handleChange(e);
					}}
				/>
				<FormCheck
					label={t('AddonsConfig:mpu6050-gyro-enabled-label')}
					type="switch"
					id="mpu6050GyroEnabledButton"
					reverse
					isInvalid={false}
					checked={Boolean(values.mpu6050GyroEnabled)}
					onChange={(e) => {
						handleCheckbox('mpu6050GyroEnabled');
						handleChange(e);
					}}
				/>
			</div>
			{getAvailablePeripherals('i2c') ? (
				<FormCheck
					label={t('Common:switch-enabled')}
					type="switch"
					id="MPU6050InputButton"
					reverse
					isInvalid={false}
					checked={
						Boolean(values.MPU6050InputEnabled) &&
						getAvailablePeripherals('i2c')
					}
					onChange={(e) => {
						handleCheckbox('MPU6050InputEnabled');
						handleChange(e);
					}}
				/>
			) : (
				<FormLabel>
					<Trans
						ns="PeripheralMapping"
						i18nKey="peripheral-toggle-unavailable"
						values={{ name: 'I2C' }}
					>
						<NavLink to="/peripheral-mapping">
							{t('PeripheralMapping:header-text')}
						</NavLink>
					</Trans>
				</FormLabel>
			)}
		</Section>
	);
};

export default MPU6050;
