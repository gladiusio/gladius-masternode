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

// Send the info about expected files and the node to use.
app.get('/master_info', function(req, res) {
  var clientAddress = req.headers['x-forwarded-for']; // Pull the client IP (behind NGINX only)
  var edgeAddress = rpcControl.closestNode(clientAddress); // Get closest node

  res.send(JSON.stringify({
    bundle: "http://" + edgeAddress + ":8080/content_bundle",
    expectedHash: "3f0a5127f481a720e36d28e233f240ee",
    name: "bundle.json"
  }))
})

rpc = rpcControl.start(app);
