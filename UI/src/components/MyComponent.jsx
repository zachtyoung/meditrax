import { useEffect, useState } from "react"

import io from "socket.io-client"

function MyComponent() {
	const [data, setData] = useState([])

	useEffect(() => {
		const socket = io("http://localhost:5555")

		socket.on("connect", () => {
			console.log("Connected to WebSocket server")
		})
		socket.on("initialData", (initialData) => {
			setData(initialData)
		})
		socket.on("newData", (newData) => {
			setData((prevData) => [...prevData, newData])
		})
		socket.on("newEvent", (newData) => {
			setData((prevData) => [...prevData, newData])
		})
		return () => {
			socket.disconnect()
		}
	}, [])

	return (
		<div>
			<code>{JSON.stringify(data)}</code>
		</div>
	)
}

export default MyComponent
