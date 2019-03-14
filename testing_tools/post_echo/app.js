var express = require("express");
var app = express();
app.use(express.json());

// echoes body of post request to client
app.post('/', function(req, res) {
    console.log(req.body);
    res.send(req.body);
});

console.log("Starting echo server...");
app.listen(8080);
