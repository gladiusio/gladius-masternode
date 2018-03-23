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
    expectedHash: "5c6ec07cf7ef04a5f9b1b53ff610f0beccec1803066071fc3df3a455850c67be",
    name: "bundle.json"
  }))
})

rpc = rpcControl.start(app);
