const { server, httpServer } = require("./server.js")

const port = process.env.PORT || 5555
httpServer.listen(port, () => console.log(`\n** Running on port ${port} **\n`))
