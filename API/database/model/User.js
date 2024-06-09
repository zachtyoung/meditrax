// models/User.js
const mongoose = require("mongoose")

// Define a schema
const userSchema = new mongoose.Schema({
	name: {
		type: String,
		required: true,
	},
	email: {
		type: String,
		required: true,
		unique: true,
	},
	password: {
		type: String,
		required: true,
	},
})

// Create a model based on the schema
const User = mongoose.model("Users", userSchema)

module.exports = User
