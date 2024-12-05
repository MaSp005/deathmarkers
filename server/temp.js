require("http").createServer((req, res) => {
  console.log("LITERALLY ANYTHING", req.url);
}).listen(8048);