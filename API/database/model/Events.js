// models/User.js
const mongoose = require("mongoose")

// Define a schema
const eventSchema = new mongoose.Schema({
	data: {
		type: String,
	},

	timestamp: {
		type: Date,
		default: Date.now,
	},
})

// Create a model based on the schema
const Events = mongoose.model("events", eventSchema)

module.exports = Events
