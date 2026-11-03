import "std/wren/core/event" for Event
import "std/wren/core/time" for Time
import "std/wren/data/json" for Json
import "std/wren/dev/log" for Log
class Main {
static on_start() {
    if(Json.exist("test.json")){
        Log.info("test.json exist")
        Json.open("test.json",10,30)
    }else{
        Log.warn("test.json dont exist")
    }
    Json.open("xx",1,1)
    if(!Json.exist("test2.json")){
        Log.info("test2.json dont exist")
        Json.create("test2.json", {
            "width": 1920,
            "height": 1080,
            "fullscreen": true,
            "name": "My Game",
            "graphics": {
                "vsync": true,
                "quality": "high"
            },
            "resolutions": [1920, 1080, 1280, 720]
        })

        Json.create("test2.json", {
            "width": 1920,
            "height": 1080,
            "fullscreen": true,
            "name": "My Game",
            "graphics": {
                "vsync": true,
                "quality": "low"
            },
            "resolutions": [1920, 1080, 1280, 720]
        })
        Json.open("test2.json",0,0)
        __x = Json.get("test2.json")
        System.print(__x)
    }

    __content = Json.get("test.json")

    Event.callback(Event.JsonSave, Fn.new {|name|
            Json.set("test.json",__content)
            Json.save("test.json")
            Log.info("test saved")
        })
    
}

static on_update(dt) {
    //__content["time"] = __content["time"] + 1
}
static on_fixed_update(dt) {}
static on_render() {}
static on_destroy() {}


}