function call_api(f,callback){
    let xmlhttp = new XMLHttpRequest();
    xmlhttp.open("GET","/api/"+f, true);
    xmlhttp.onreadystatechange=function(){
         if(xmlhttp.readyState==4 && xmlhttp.status==200){
              callback(JSON.parse(xmlhttp.responseText));
         }
    }
    xmlhttp.send();
};

//document.addEventListener("DOMContentLoaded", function(){
window.onload = function(){
    window.requestAnimationFrame = (function(){
        return window.requestAnimationFrame		||
            window.webkitRequestAnimationFrame	||
            window.mozRequestAnimationFrame		||
            window.oRequestAnimationFrame		||
            window.msRequestAnimationFrame		||
            function(callback, element){
                window.setTimeout(callback, 1000 / 30);
            };
    })();

//---------------------Flailing bar---------------------------------

    let flailing_bar = {gamma: 0,
                        canvas: document.createElement("canvas")};
    (function(right_canvas){
        let top_section = document.createElement("section");
        top_section.classList.add("top_section");
        document.body.appendChild(top_section);

        let empty_relative_div = document.createElement("div");
        empty_relative_div.classList.add("top_section_empty_relative_div");
        top_section.appendChild(empty_relative_div);

        let left_fill = document.createElement("div");
        left_fill.classList.add("top_section_left_fill");
        empty_relative_div.appendChild(left_fill);

        right_canvas.classList.add("top_section_right_canvas");
        empty_relative_div.appendChild(right_canvas);

        let middle_text = document.createElement("h1");
        middle_text.classList.add("top_section_middle_text");
        middle_text.innerHTML = "A π server です！";
        empty_relative_div.appendChild(middle_text);
    })(flailing_bar.canvas);

    function render_flailing_bar(canvas, data) {
        let ctx = canvas.getContext("2d");
        let line_thickness = (canvas.height/2)/3 - 0.3;
        let theta_thickness_offset = Math.asin(line_thickness/canvas.height);
        let theta_max = Math.PI/6;
        let gamma_speed = (Math.PI/60)/4;
        let rectangles = [[0,0, canvas.width/2,0, canvas.width,canvas.height, canvas.width/2,line_thickness, 0,line_thickness],
                          [0,line_thickness, canvas.width/2,line_thickness, canvas.width,canvas.height, canvas.width/2,line_thickness*2, 0,line_thickness*2],
                          [0,line_thickness*2, canvas.width/2,line_thickness*2, canvas.width,canvas.height, canvas.width/2,line_thickness*3, 0,line_thickness*3]];

        theta = theta_max*Math.sin(data.gamma);
        data.gamma += gamma_speed;
        if(data.gamma > Math.PI*2){
            data.gamma -= Math.PI*2;
        }
        ctx.clearRect(0, 0, canvas.width, canvas.height);
        for(let i=0; i<3; i++){
            ctx.beginPath();
            ctx.moveTo(rectangles[i][0],rectangles[i][1]);
            ctx.quadraticCurveTo(rectangles[i][2],rectangles[i][3], rectangles[i][4]*Math.cos(theta),rectangles[i][5]*Math.sin(theta)+line_thickness*i);
            ctx.lineTo(rectangles[i][4]*Math.cos(theta+theta_thickness_offset),rectangles[i][5]*Math.sin(theta+theta_thickness_offset)+line_thickness*i);
            ctx.quadraticCurveTo(rectangles[i][6],rectangles[i][7], rectangles[i][8],rectangles[i][9]);
            ctx.lineTo(rectangles[i][0],rectangles[i][1]);
            ctx.closePath();
            ctx.fillStyle = "black";
            ctx.strokeStyle = "black";
            ctx.fill();
            ctx.stroke();
        }
    }

    (function animationLoop(){
        render_flailing_bar(flailing_bar.canvas, flailing_bar);
        requestAnimationFrame(animationLoop);
    })();
//--------------Content----------------------------------------
    (function(){
        let page_content = document.createElement("section");
        document.body.appendChild(page_content);
        page_content.classList.add("page_content");
    //-----------------Sidebar--------------------------------------
        (function(){
            let page_navigation = document.createElement("section");
            page_content.appendChild(page_navigation);
            page_navigation.classList.add("nav");

            let background = document.createElement("section");
            background.classList.add("nav_bg");
            page_navigation.appendChild(background);

            call_api("get_paths_to_pages",function(res){
                let paths_to_pages = res;
                call_api("get_paths_to_data",function(res){
                    let paths_to_data = res;

                    paths_to_pages.forEach(function(page, i){
                        let link = document.createElement("a");
                        let br = document.createElement("br");
                        let button = document.createElement("div");
                        let img = document.createElement("img");

                        page_navigation.appendChild(link);
                        page_navigation.appendChild(br);
                        link.appendChild(button);
                        button.appendChild(img);

                        button.classList.add("nav_button_shadow");
                        img.classList.add("nav_button_popout");

                        button.addEventListener("mouseenter", function(event){
                            background.classList.add("nav_button_shadow-hover-nav_bg");
                        });
                        button.addEventListener("mouseleave", function(event){
                            background.classList.remove("nav_button_shadow-hover-nav_bg");
                        });

                        link.href = page;
                        //TODO(bjorn): Remove this magic number. It is there because all adresses starts with root "/".
                        let a = 1;
                        let b = page.substr(a).indexOf("/");
                        if(b == -1){
                            let main_page_name = "Main Page";
                            button.appendChild(document.createTextNode(main_page_name));
                            img.src = "/data/" + main_page_name + ".png";
                        }else{
                            button.appendChild(document.createTextNode(page.substr(a,b)));
                            img.src = "/data/" + page.substr(a,b) + ".png";
                        }
                    });
                });
            });
        })();
    //--------------Main------------------------------------------
        (function(){
            document.body.main = document.createElement("div");
            document.body.main.classList.add("main");
            page_content.appendChild(document.body.main);
        })();
    })();
//--------------Footer-----------------------------------------
    (function(){
        let footer = document.createElement("div");
        footer.classList.add("footer");
        footer.appendChild(document.createTextNode("A site for cool people™"));
        document.body.appendChild(footer);
    })();

    if(document.main_callback != undefined){
        document.main_callback();
    }
}
//});
