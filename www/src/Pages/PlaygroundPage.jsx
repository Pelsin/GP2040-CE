import { useState, useEffect, useRef } from 'react';
import { Container, Row, Col, Card, Button, Badge, Alert } from 'react-bootstrap';

export default function PlaygroundPage() {
	const [isConnected, setIsConnected] = useState(false);
	const [gamepadState, setGamepadState] = useState(null);
	const [connectionStatus, setConnectionStatus] = useState('Disconnected');
	const [error, setError] = useState(null);
	const wsRef = useRef(null);

	const connectWebSocket = () => {
		try {
			setError(null);
			setConnectionStatus('Connecting...');

			const ws = new WebSocket('ws://192.168.7.1:8080/gpio-state');
			wsRef.current = ws;

			ws.onopen = () => {
				setIsConnected(true);
				setConnectionStatus('Connected');
				console.log('WebSocket connected to gamepad monitor');
			};

			ws.onmessage = (event) => {
				try {
					console.log('Raw WebSocket message:', event.data);
					const data = JSON.parse(event.data);
					console.log('Parsed data:', data);

					if (data.type === 'welcome') {
						console.log('Received welcome message:', data.message);
					} else if (data.type === 'gpio_state') {
						setGamepadState(data);
					} else if (data.type === 'debug') {
						console.log('Debug message:', data.message);
					} else {
						console.log('Unknown message type:', data.type);
					}
				} catch (err) {
					console.error('Error parsing gamepad data:', err);
					setError(`Error parsing data: ${err.message}`);
				}
			};

			ws.onclose = (event) => {
				setIsConnected(false);
				setConnectionStatus('Disconnected');
				console.log('WebSocket disconnected:', event.code, event.reason);
				if (event.code !== 1000) {
					setError(`Connection closed unexpectedly (code: ${event.code})`);
				}
			};

			ws.onerror = (error) => {
				console.error('WebSocket error:', error);
				setError('Connection failed. Make sure GP2040-CE is in config mode.');
			};

		} catch (err) {
			setError(`Connection failed: ${err.message}`);
			setConnectionStatus('Disconnected');
		}
	};

	const disconnectWebSocket = () => {
		if (wsRef.current) {
			wsRef.current.close(1000, 'User requested disconnect');
			wsRef.current = null;
		}
	};

	useEffect(() => {
		return () => {
			disconnectWebSocket();
		};
	}, []);

	return (
		<Container fluid className="py-4">
			<Row className="justify-content-center">
				<Col lg={10} xl={8}>
					<h1 className="mb-4 text-center">GPIO Pin Monitor</h1>

					<Card className="mb-4 shadow-sm">
						<Card.Header className="d-flex justify-content-between align-items-center">
							<div>
								<h5 className="mb-1">WebSocket Connection</h5>
								<div className="d-flex align-items-center">
									<span className="me-2">Status:</span>
									<Badge
										bg={isConnected ? 'success' : connectionStatus === 'Connecting...' ? 'warning' : 'danger'}
										className="me-2"
									>
										{connectionStatus}
									</Badge>
								</div>
							</div>
							<Button
								variant={isConnected ? 'danger' : 'success'}
								onClick={isConnected ? disconnectWebSocket : connectWebSocket}
								disabled={connectionStatus === 'Connecting...'}
							>
								{isConnected ? 'Disconnect' : 'Connect'}
							</Button>
						</Card.Header>
						{error && (
							<Card.Body>
								<Alert variant="danger" className="mb-0">
									{error}
								</Alert>
							</Card.Body>
						)}
					</Card>

					{gamepadState && (
						<Card className="shadow-sm">
							<Card.Header>
								<h4 className="mb-0">Live GPIO State</h4>
							</Card.Header>
							<Card.Body>
								<Row>
									<Col md={12} className="mb-4">
										<h5 className="mb-3">Pressed Pins</h5>
										{gamepadState.pressedPins && gamepadState.pressedPins.length > 0 ? (
											<div className="d-flex flex-wrap">
												{gamepadState.pressedPins.map(pin => (
													<Badge key={pin} bg="success" className="me-2 mb-2 p-2">
														GPIO {pin}
													</Badge>
												))}
											</div>
										) : (
											<Badge bg="secondary" className="p-2">No pins pressed</Badge>
										)}
									</Col>
								</Row>
							</Card.Body>
						</Card>
					)}
				</Col>
			</Row>
		</Container>
	);
}
