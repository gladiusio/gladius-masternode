function httpGet(theUrl) {
  var xmlHttp = new XMLHttpRequest();
  xmlHttp.open("GET", theUrl, false); // false for synchronous request
  xmlHttp.send(null);
  return xmlHttp.responseText;
}

function fetchAndLoadEdge() {
  var masterInfo = JSON.parse(httpGet("/master_info")); // Information about node and files
  console.log(masterInfo);
  var edgeData = JSON.parse(httpGet(masterInfo.bundle))[masterInfo.name];

  // Verify the file matches
  for (let dataKey of Object.keys(edgeData)) {
    if (md5(edgeData[dataKey]) === masterInfo.expectedHashes[dataKey]) {
      var type = "";
      var srcString = "";

      if (dataKey.endsWith(".js")) {
        type = "script";
        srcString = "data:text/javascript;base64,";
      } else if (dataKey.endsWith(".jpg")) {
        type = "img";
        srcString = "data:image/jpg;base64,";
      }

      // Create a tag and attach the data
      var tag = document.createElement(type);
      tag.src = srcString + edgeData[dataKey];

      // Insert the script as a child to this script
      document.getElementById(dataKey).appendChild(tag);
    } else {
      alert("Edge node delivered non matching content!")
    }
  }
}

fetchAndLoadEdge();
