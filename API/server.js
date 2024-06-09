const express = require("express")
const mongoose = require("./mongoose")
const cors = require("cors")
const server = express()
const httpServer = require("http").createServer(server) // Create HTTP server
const io = require("socket.io")(httpServer, {
	cors: {
		origin: "http://localhost:5173",
		methods: ["GET", "POST"],
	},
}) // Attach Socket.IO to HTTP server

server.use(cors())
server.use(express.json())
server.use(function (req, res, next) {
	res.header("Access-Control-Allow-Origin", "*") // update to match the domain you will make the request from
	res.header(
		"Access-Control-Allow-Headers",
		"Origin, X-Requested-With, Content-Type, Accept, Authorization"
	)
	res.header(
		"Access-Control-Allow-Methods",
		"GET, POST, OPTIONS, PUT, DELETE"
	)
	next()
})

const User = require("./database/model/User")
const Events = require("./database/model/Events")

server.get("/users", async (req, res) => {
	try {
		const users = await User.find({})
		res.json(users)
	} catch (err) {
		console.error(err)
		res.status(500).send(err)
	}
})
server.get("/events", async (req, res) => {
	try {
		const events = await Events.find({})
		res.json(events)
	} catch (err) {
		console.error(err)
		res.status(500).send(err)
	}
})

server.post("/users", async (req, res) => {
	try {
		const { name, email, password } = req.body

		// Validate request body
		if (!name || !email || !password) {
			return res.status(400).json({ message: "All fields are required" })
		}

		// Create a new user
		const user = new User({ name, email, password })
		await user.save()

		// Emit event to connected clients (Socket.IO)
		io.emit("newData", user)

		res.status(201).json(user)
	} catch (err) {
		console.error(err)
		res.status(500).json({ message: "Internal server error" })
	}
})

server.post("/events", async (req, res) => {
	try {
		const { data } = req.body

		// Validate request body
		if (!data) {
			return res.status(400).json({ message: "All fields are required" })
		}

		// Create a new user
		const event = new Events({ data })
		await event.save()

		// Emit event to connected clients (Socket.IO)
		io.emit("newEvent", event)

		res.status(201).json(event)
	} catch (err) {
		console.error(err)
		res.status(500).json({ message: "Internal server error" })
	}
})

// Socket.IO connection handling
io.on("connection", async (socket) => {
	console.log("A client connected")
	try {
		const users = await User.find({})
		const events = await Events.find({})
		socket.emit("initialData", [users, events])
	} catch (err) {
		console.error(err)
	}
})

module.exports = { server, httpServer } // Export both Express server and HTTP server
