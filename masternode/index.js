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
    expectedHashes: {
      "alert.js": "30874C663E10FB4EA67FDE881D4B263F",
      "cat.jpg": "C67A60D24F26C59D1D674CB2D1714902"
    },
    name: "bundle.json"
  }))
})

rpc = rpcControl.start(app);
