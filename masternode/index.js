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
    expectedHash: "dc25297d5001529eefcf7a6ec354409b",
    name: "bundle.json"
  }))
})

rpc = rpcControl.start(app);
