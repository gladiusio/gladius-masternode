function fetchAndLoadEdge() {
  var expectedHashes = {
    script1: "13E088365949F8A2D50443B58D28626E"
  }; // Loaded from master node

  var edgeData = {
    script1: {
      base64: "YWxlcnQoJ2FzYWRhc2QnKTsK"
    }
  }; // Would normally be fetched from edge node

  // Verify the file matches
  for (let dataKey of Object.keys(edgeData)) {
    if (md5(edgeData[dataKey].base64) === expectedHashes[dataKey]) {
      // Create a script tag and attach the base64 data from the edge node if valid
      var script = document.createElement('script');
      script.src = "data:text/javascript;base64," + edgeData[dataKey].base64;

      // Insert the script as a child to this script
      document.getElementById(dataKey).appendChild(script);
    } else {
      alert("Edge node delivered non matching content!")
    }
  }
}

fetchAndLoadEdge();
