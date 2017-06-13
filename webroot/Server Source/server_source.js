function url_get(url, callback){
    let xmlhttp = new XMLHttpRequest();
    xmlhttp.open("GET",url);
    xmlhttp.onreadystatechange=function(){
         if(xmlhttp.readyState==4 && xmlhttp.status==200){
              callback(xmlhttp.responseText);
         }
    }
    xmlhttp.send();
};

document.main_callback = function(){
    let code = document.createElement("div");
    code.classList.add("code");
    document.body.main.appendChild(code);

    url_get("/node.js", function(sourcecode){
        code.innerHTML = sourcecode;
    });
}
