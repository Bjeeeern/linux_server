document.main_callback = function(){
    function isEmpty(obj) {
        return Object.keys(obj).length === 0;
    };
    function setCookieObject(object, exdays){
        let date = new Date();
        date.setTime(date.getTime() + (exdays*24*60*60*1000));
        let expires = "expires="+ date.toUTCString();

        Object.keys(object).forEach(function (key) {
            document.cookie = key + "=" + object[key] + ";" + expires;
        });
    };
    function getCookieObject(){
        let ca = document.cookie.split(";");
        if(ca[0] == ""){
            return;
        }
        for(let i = 0; i <ca.length; i++) {
            let c = ca[i].split("=");
            this[c[0].trim()] = c[1].trim();
        }
    };

    let canvas = document.createElement("canvas");
    canvas.width = document.body.main.clientWidth;
    canvas.height = document.body.main.clientHeight;
    canvas.classList.add("canvas");
    document.body.main.appendChild(canvas);

    let camera = {};
    camera.x = 300;
    camera.y = 200;

    let entities = [];
    let player = {};
    player.w = 40;
    player.h = 40;
    player.speed = 4;

    let cookie = new getCookieObject();
    if(!isEmpty(cookie)){
        camera.x = Number(cookie.x);
        camera.y = Number(cookie.y);
    }
    window.onbeforeunload = function (event) {
        setCookieObject({x: camera.x, y: camera.y}, 1);
    };

    let goal = {};
    goal.x = 400;
    goal.y = 400;
    goal.w = 80;
    goal.h = 60;
    goal.text = "Goal";
    goal.finish_text = "You win!!";
    entities.push(goal);

    let won = false;

    let input = {};
    let key_index = { "left":37, "up":38, "right":39, "down":40 };
    Object.keys(key_index).map(function(key, index){
        let value = key_index[key];
        input[key] = false;
    });
    document.addEventListener("keydown", function(event){
        let code = event.keyCode;
        Object.keys(key_index).map(function(key, index){
            let value = key_index[key];
            if(code === value){
                input[key] = true;
            }
        });
    });
    document.addEventListener("keyup", function(event){
        let code = event.keyCode;
        Object.keys(key_index).map(function(key, index){
            let value = key_index[key];
            if(code === value){
                input[key] = false;
            }
        });
    });

    (function update(){

        let ctx = canvas.getContext("2d");
        let h = canvas.height;
        let w = canvas.width;

        ctx.clearRect(0, 0, w, h);

        if(camera.x > goal.x - (goal.w /2) &&
            camera.y > goal.y - (goal.h /2) &&
            camera.x < goal.x + (goal.w /2) &&
            camera.y < goal.y + (goal.h /2) )
        {
            won = true;
        }

        if(won){
            ctx.strokeStyle="#ABCDEF"
            ctx.strokeRect(goal.x - (goal.w /2), goal.y - (goal.h /2), goal.w, goal.h);
            ctx.font = "50px serif";
            ctx.fillText(goal.finish_text, goal.x - (goal.w /2), goal.y - (goal.h /2));
        }else{
            if(input.left){
                camera.x -= player.speed;
            }
            if(input.right){
                camera.x += player.speed;
            }
            if(input.up){
                camera.y -= player.speed;
            }
            if(input.down){
                camera.y += player.speed;
            }

            ctx.strokeStyle="green";//"#FFFFFF"
            ctx.strokeRect(goal.x - (goal.w /2), goal.y - (goal.h /2), goal.w, goal.h);
            ctx.font = "38px serif";
            ctx.fillText(goal.text, goal.x - (goal.w /2), goal.y);
        }

        ctx.fillStyle="#FF0000";
        ctx.fillRect(player.x - (player.w /2), player.y - (player.h /2), player.w, player.h);

        requestAnimationFrame(update);
    })();
}
