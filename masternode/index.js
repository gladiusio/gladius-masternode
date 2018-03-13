var express = require('express');
var geoip = require('geoip-lite');
var rpcControl = require('./controller/manager');

app = express();

app.set('view engine', 'pug');
app.set('views', './views');

// Create a demo route
app.get('/', function(req, res) {
  var ip = req.headers['x-forwarded-for'];

  console.log(rpcControl.closestNode(ip));
  res.render('index.pug', {
    title: 'Gladius Demo',
    message: 'Your IP is: ' + ip
  })
})

rpc = rpcControl.start(app);
