var express = require('express');
var geoip = require('geoip-lite');
var path = require('path');
var rpcControl = require('./controller/manager');

app = express();

app.set('view engine', 'pug');
app.set('views', './views');
app.use(express.static(__dirname + '/public'));

// Create a demo route
app.get('/', function(req, res) {
  res.sendFile(path.join(__dirname, 'public/views/index.html'));
})

app.get('/master_info', function(req, res) {
  var clientAddress = req.headers['x-forwarded-for']; // Pull the client IP (behind NGINX only)
  var edgeAddress = rpcControl.closestNode(clientAddress); // Get closest node

  res.send(JSON.stringify({
    bundle: "http://" + edgeAddress + ":8080/content_bundle",
    expectedHash: "e892484a5362708806674ee9d5516d59",
    name: "bundle.json"
  }))
})

rpc = rpcControl.start(app);
