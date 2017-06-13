var http = require("http");
var args = process.argv.slice(2);
var ip = args[0] || "localhost";
var port = args[1] || 8000;
var fs = require("fs");

String.prototype.endsWith = function(suffix){
    return this.indexOf(suffix, this.length - suffix.length) !== -1;
};
String.prototype.beginsWith = function(suffix){
    return this.indexOf(suffix, 0) == 0;
};
String.prototype.contains = function(char){
    return this.indexOf(char) > -1;
};
String.prototype.argAfter = function (nominal) {
    return this.substr(nominal.length, this.length-1);
};

var api = {
    get_paths_to_pages : function(req, res){
        console.log("get_paths_to_pages called");
        if(req.method != "GET"){
            console.log("funky html?");
        }
        res.writeHead(200);
        let paths = function all_indexes_in_folder(folder){
            let result = [];
            let files = fs.readdirSync(folder);
            files.forEach(function(file){
                if(file.endsWith(".html")){
                    if(folder.beginsWith(".")){
                        folder_sans_dot = folder.slice(1);
                    }else{
                        folder_sans_dot = folder;
                    }
                    result.push(folder_sans_dot + "/" + file);
                }
                if( (!file.contains(".")) && (file != "data") ){
                    result = result.concat(all_indexes_in_folder(folder + "/" + file));
                }
            });
            return result;
        }(".");
        res.write(JSON.stringify(paths));
        res.end();
    },
    get_paths_to_data : function(req, res){
        console.log("get_paths_to_data called");
        if(req.method != "GET"){
            console.log("funky html?");
        }
        res.writeHead(200);
        let paths = function all_indexes_in_folder(folder){
            let result = [];
            let files = fs.readdirSync(folder);
            files.forEach(function(file){
                if(file.contains(".")){
                    result.push(folder + "/" + file);
                }
                else{
                    result = result.concat(all_indexes_in_folder(folder + "/" + file));
                }
            });
            return result;
        }("./data");
        res.write(JSON.stringify(paths));
        res.end();
    }
};

var server = http.createServer()

server.listen(port, ip, function(){
    console.log("Server running on %s:%s", ip, port);
});

server.on("request",function(req, res){
    req.url = decodeURI(req.url);
    if((req.url.endsWith(".html") ||
       req.url.endsWith(".js") ||
       req.url.endsWith(".css")) && req.method == "GET")
    {
        fs.readFile("." + req.url, "utf8", function(err, data){
            if(err){
                res.writeHead(404, {'Content-type':'text/html'})
                console.log(err + req.url);
                res.end("source not found");
            }
            else{
                res.writeHead(200);
                res.write(data);
                res.end();
            }
        });
    }
    else if(req.url.beginsWith("/api/"))
    {
        if(api[req.url.argAfter("/api/")] === undefined){
            res.writeHead(400, {'Content-type':'text/html'})
            console.log("bad api call ["+ req.url.argAfter("/api/") +"]");
            res.end("Bad api call");
        }else{
            api[req.url.argAfter("/api/")](req, res);
        }
    }
    else if(req.url.beginsWith("/data/")){
        fs.readFile("." + req.url, function (err, data) {
            if (err) {
                res.writeHead(400, {'Content-type':'text/html'})
                console.log(err);
                res.end("No such data");
            } else {
                res.writeHead(200);
                res.write(data);
                res.end();
            }
        });
    }
    else{
        res.writeHead(200, {"Content-Type":"text/html"});
        fs.readFile("./index.html", "utf8", function(err, data){
            if(err){
                res.writeHead(400, {'Content-type':'text/html'})
                console.log(err);
                res.end("Site is down");
            }
            res.write(data);
            res.end();
        });
    }
});
