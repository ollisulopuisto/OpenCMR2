// Optional enhancements are per launch. Never save selections into the profile.
#ifndef OPENCMR2_ENHANCEMENTS_H
#define OPENCMR2_ENHANCEMENTS_H
#include <string>
#include <map>
#include <cstdlib>
#include <utility>
struct EnhancementOptions {
    std::string captureDir;int captureInterval=5,captureCount=7;
    std::string trees, spectators, names, cars, modsDir, carNames, roads, textures;
    bool installDependencies=false,prepareOnly=false;
    std::string testPreset;
    std::map<std::string,int> overrides;
    bool Consume(int &i,int argc,char **argv,std::string &error) {
        std::string key=argv[i];
        auto equal=key.find('=');
        std::string inlineValue;
        if(equal!=key.npos){inlineValue=key.substr(equal+1);key.resize(equal);}
        if(equal==key.npos && key=="--prepare-only"){prepareOnly=true;return true;}
        if(equal==key.npos && key=="--install-enhancement-deps"){installDependencies=true;return true;}
        if(key!="--textures"&&key!="--roads"&&key!="--tree-wind"&&key!="--car-names"&&key!="--trees"&&key!="--spectators"&&key!="--driver-names"&&key!="--cars"&&key!="--mods-dir"&&key!="--effect"&&key!="--attract-capture"&&key!="--capture-interval"&&key!="--capture-count"&&key!="--speed-fov"&&key!="--speed-motes-stretch"&&key!="--speed-motes-amount"&&key!="--speed-motes-size"&&key!="--dof-blur-max-px"&&key!="--speed-streak"&&key!="--speed-hold"&&key!="--cockpit-eye-shift"&&key!="--camera-move-chase"&&key!="--camera-move-cockpit"&&key!="--camera-move-car"&&key!="--fov-dolly-curve"&&key!="--fov-preview"&&key!="--dof-focus-car"&&key!="--test-preset")return false;
        if(equal==std::string::npos && i+1>=argc){error="Missing value for "+key;return true;}
        std::string value=equal==std::string::npos?argv[++i]:inlineValue;
        if(key=="--textures"){if(value!="original"&&value!="france1")error="--textures expects original or france1";else textures=value;}
        if(key=="--roads"){if(value!="original"&&value!="curved")error="--roads expects original or curved";else roads=value;}
        if(key=="--tree-wind"){if(value!="on"&&value!="off")error="--tree-wind expects on or off";else overrides["video.tree_wind"]=value=="on";}
        if(key=="--car-names"){if(value!="original"&&value!="skin")error="--car-names expects original or skin";else carNames=value;}
        if(key=="--trees") {if(value!="original"&&value!="shell"&&value!="crossed")error="--trees expects original, shell or crossed";else trees=value;}
        if(key=="--spectators"){if(value!="original"&&value!="animated")error="--spectators expects original or animated";else spectators=value;}
        if(key=="--driver-names"){if(value!="original"&&value!="real"&&value!="sidecar")error="--driver-names expects original, real or sidecar";else names=value;}
        if(key=="--effect"){
            auto at=value.find('=');std::string name=value.substr(0,at),v=at==value.npos?"":value.substr(at+1);
            static const char *allowed[]={"dust_plume","body_dirt","dust_volume","volumetric_clouds","spatial_aa","metalfx_spatial","motion_blur","bloom","particles","shadows","depth_of_field","god_rays","color_grading","temporal_aa","speed_motes","debug_object_mask","vapor_trails","speed_lines","tunnel_lines"};
            bool known=false;for(auto key:allowed)if(name==key)known=true;
            if(!known||(v!="on"&&v!="off"))error="--effect expects a listed NAME=on/off";
            else overrides["video."+name]=v=="on";
        }
        if(key=="--speed-fov"||key=="--speed-motes-stretch"||key=="--speed-motes-amount"||key=="--speed-motes-size"||key=="--dof-blur-max-px"){
            char *end=nullptr;long n=strtol(value.c_str(),&end,10);
            int hi=key=="--speed-fov"?150:key=="--speed-motes-stretch"?100:key=="--dof-blur-max-px"?40:key=="--speed-motes-size"?800:300,lo=key=="--dof-blur-max-px"?1:key=="--speed-motes-size"?50:0;
            if(value.empty()||*end||n<lo||n>hi)error=key+" expects "+std::to_string(lo)+".."+std::to_string(hi);
            else overrides[key=="--speed-fov"?"video.speed_fov":key=="--speed-motes-stretch"?"video.speed_motes_stretch":key=="--dof-blur-max-px"?"video.dof_blur_max_px":key=="--speed-motes-size"?"video.speed_motes_size":"video.speed_motes_amount"]=(int)n;
        }
        if(key=="--cockpit-eye-shift"){
            char *end=nullptr;long n=strtol(value.c_str(),&end,10);   // signed: negative moves the eye the other way
            if(value.empty()||*end||n<-100||n>100)error=key+" expects -100..100";else overrides["video.cockpit_eye_shift"]=(int)n;
        }
        if(key=="--fov-dolly-curve"){
            if(value!="0"&&value!="1")error="--fov-dolly-curve expects 0 or 1";else overrides["video.fov_dolly_curve"]=value=="1"?1:0;
        }
        if(key=="--camera-move-chase"||key=="--camera-move-cockpit"||key=="--camera-move-car"){
            char *end=nullptr;long n=strtol(value.c_str(),&end,10);   // signed: positive forward, negative back, 0.05 model units per step
            if(value.empty()||*end||n<-200||n>200)error=key+" expects -200..200";else overrides[key=="--camera-move-chase"?"video.camera_move_chase":key=="--camera-move-cockpit"?"video.camera_move_cockpit":"video.camera_move_car"]=(int)n;
        }
        if(key=="--speed-hold"||key=="--dof-focus-car"){
            // 0/1. --speed-hold 0 makes --test-preset overdo run its speed effects (FOV, tunnel, streak) at the real car speed instead of
            // pretending full speed; --dof-focus-car 0 keeps the fixed focus plane under the preset. An explicit flag always wins.
            if(value!="0"&&value!="1")error=key+" expects 0 or 1";else overrides[key=="--speed-hold"?"video.speed_fov_hold":"video.dof_focus_car"]=value=="1"?1:0;
        }
        if(key=="--fov-preview"){
            // Debug view of the absolute video.fov number (legacy scale, higher = wider; not a measured angle): 0 = off, else 20..150.
            // Per launch only. Speed FOV % is a different, relative setting.
            char *end=nullptr;long n=strtol(value.c_str(),&end,10);
            if(value.empty()||*end||(n!=0&&(n<20||n>150)))error="--fov-preview expects 0 (off) or 20..150";else overrides["video.fov_preview"]=(int)n;
        }
        if(key=="--speed-streak"){
            char *end=nullptr;long n=strtol(value.c_str(),&end,10);
            if(value.empty()||*end||n<0||n>100)error="--speed-streak expects 0..100";else overrides["video.speed_streak"]=(int)n;
        }
        if(key=="--test-preset"){if(value!="overdo"&&value!="off")error="--test-preset expects overdo or off";else testPreset=value;}
        if(key=="--attract-capture"){if(value.empty())error="--attract-capture needs an output folder";else captureDir=value;}
        if(key=="--capture-interval"||key=="--capture-count"){
            char *end=nullptr;long n=strtol(value.c_str(),&end,10);bool interval=key=="--capture-interval";
            if(value.empty()||*end||n<1||n>(interval?60:120))error=key+(interval?" expects 1..60 seconds":" expects 1..120 shots");
            else (interval?captureInterval:captureCount)=(int)n;
        }
        if(key=="--cars"){if(value.empty())error="--cars needs a nonempty alternative list";else cars=value;}
        if(key=="--mods-dir"){if(value.empty())error="--mods-dir needs a folder";else modsDir=value;}

        return true;
    }
    // Exaggerated diagnostic look (per launch, nothing saved). Explicit flags always win.
    void ApplyTestPreset(){
        if(testPreset!="overdo")return;
        static const std::pair<const char*,int> kOverdo[]={{"video.speed_motes",0},
            {"video.speed_fov",40},{"video.speed_fov_hold",1},{"video.depth_of_field",1},{"video.dof_blur",100},{"video.dof_blur_max_px",16},{"video.tunnel_vision",100},{"video.motion_blur",1},{"video.motion_blur_amount",60},{"video.speed_streak",100},{"video.dof_focus_car",1}};
        for(auto&e:kOverdo)if(!overrides.count(e.first))overrides[e.first]=e.second;
    }
    void Apply(){
        ApplyTestPreset();
        if(!trees.empty()&&!overrides.count("video.tree_wind"))overrides["video.tree_wind"]=trees!="original";
        if(!carNames.empty())overrides["video.skin_car_names"]=carNames=="skin";
        // A generated pack contains both kinds; activate only requested kinds.
        if(NeedsPack()&&trees.empty())overrides["video.tree_overrides"]=0;
        if(NeedsPack()&&spectators.empty())overrides["video.spectator_overrides"]=0;
        if(!trees.empty())overrides["video.tree_overrides"]=trees!="original";
        if(!spectators.empty())overrides["video.spectator_overrides"]=spectators!="original";
        if(trees=="shell"||trees=="crossed"||spectators=="animated")overrides["video.scenery_overrides"]=1;
        if(!names.empty()){overrides["video.driver_name_overrides"]=names!="original";overrides["video.real_driver_names"]=names=="real";}
    }
    bool NeedsPack()const{return trees=="shell"||trees=="crossed"||spectators=="animated";}
};
inline const char *EnhancementHelp(){return
"OpenCMR2 options (per launch, no saved selection changes):\n"
"  Valued enhancement flags accept --name value or --name=value.\n"
"  --data PATH                 Owned installed PC game data\n"
"  --user-dir PATH             Explicit DIFFERENT saves/settings profile\n"
"  --scenery-dir PATH          Existing read-only sidecar pack\n"
"  --trees original|shell|crossed\n"
"                              Prepare/cache selected experimental tree mode\n"
"                              Only3high-source mappings; shell is tiered;\n"
"                              crossed is fixed cutouts, not full3Dtrees\n"
"  --textures original|france1 Exact-source France1 opaque road2x filter\n"
"                              Only t_1a/b high+low; not restored detail\n"
"                              Other stages stay original; native unverified\n"
"  --roads original|curved      Experimental Ballon paired render/contact curve\n"
"                              Exact original source only; others stay original\n"
"                              FIN barriers unchanged; native handling unverified\n"
"                              Requires Python3.10+ with numpy; no auto download\n"
"  --tree-wind on|off          2D foliage sway, fixed bases; default on with trees\n"
"                              Hash-verified individual tree cards only\n"
"  --spectators original|animated\n"
"                             Four limb poses on10France atlases;38others subtle sway\n"
"  --driver-names original|real|sidecar\n"
"                              Real=15extracted display names, no save edits\n"
"                              Sidecar=existing validated driver-names.tsv\n"
"  --cars LIST                 Comma-separated owned ZIP alternatives (below)\n"
"                              original=use unchanged selected data\n"
"  --car-names skin|original   Selected skin labels (default skin); display only\n"
"                              Original restores labels, not replacement assets\n"
"  --mods-dir PATH             Folder containing original downloaded ZIPs\n"
"                              Required with --cars, no assets downloaded\n"
"                              Hash-checked replacement models/info/textures\n"
"                              Not native-tested; may include physics changes\n"
"  --install-enhancement-deps   Allow uv to create an isolated Python3.12\n"
"                              environment and install Pillow/numpy (network)\n"
"  --effect NAME=on|off        Per-launch existing optional GPU effects:\n"
"                              dust_plume,body_dirt,dust_volume,volumetric_clouds,spatial_aa,metalfx_spatial,\n"
"                              motion_blur,bloom,particles,shadows,\n"
"                              depth_of_field,god_rays,color_grading,temporal_aa,speed_motes\n"
"                              Requested is not proof of backend support;\n"
"                              see runtime log. temporal_aa = PARTIAL stage (SDL_GPU only,\n"
"                              camera reprojection, no object velocity), default off.\n"
"  --speed-fov 0..150          Widen the regular cameras' view with speed (percent); default 0 = off\n"
"  --speed-streak 0..100       Directional radial speed streak (SDL_GPU only); default 0 = off\n""  --speed-hold 0|1            Hold speed effects at full speed (overdo sets 1); 0 = real car speed\n"
"  --fov-preview 0|20..150     Debug: try an absolute video.fov number (legacy scale, 70 = original, higher = wider, not an angle); F10 row too; 0 = off\n"
"  --dof-focus-car 0|1         Focus depth of field on the followed car (SDL_GPU; overdo sets 1); default 0 = fixed plane\n"

"  --speed-motes-stretch 0..100 Stretch speed motes into radial streaks; needs --effect speed_motes=on\n"
"  --speed-motes-amount 0..300 Percent of the normal mote spawn rate (live cap 64 at 100, up to 192 at 300)\n"
"  --speed-motes-size 50..800  Mote size in percent (default 100)\n"
"  --dof-blur-max-px 1..40     Blur radius in pixels when dof_blur is 100 (default 4)\n"
"  --camera-move-chase|--camera-move-cockpit|--camera-move-car -200..200  Move that camera family along its forward axis by this many 0.05 model unit steps at full speed, blending from 0 standing still; + forward, - back (default 0; visual only)\n"
"  --fov-dolly-curve 0|1  Experimental: 1 spreads the chase dolly (same 1.0 to 0.70 limits) linearly over widening 1.0 to 2.0 instead of reaching 0.70 at about x1.3 (default 0 = current curve; visual only)\n"
"  --cockpit-eye-shift -100..100  Cockpit camera: move the eye along its forward axis as the view widens (default 0 = off; sign tuned natively; visual only)\n"
"  --test-preset overdo        Exaggerated diagnostic look for speed lines/streak, FOV, DoF, tunnel vision (legacy motes off; add --effect speed_motes=on)\n"
"                              and motion blur. Explicit flags override it; nothing is saved\n"
"  --attract-capture DIR       Start the attract demo at once, save PNGs of the frame at\n"
"                              stage-clock 0, N, 2N... s into DIR (+ capture.csv), then quit\n"
"  --capture-interval SEC      Seconds of stage clock between shots (1..60, default 5)\n"
"  --capture-count N           Number of shots (1..120, default 7 = 0..30 s)\n"
"  --prepare-only              Prepare/validate selections, then exit\n"
"  --help, --help-enhancements  Show this list without starting the game\n"
"Car alternatives (slot replacement, NOT native-tested):\n"
"  205-dakar-1987 OR 205-finland-1986\n"
"  subaru-spain-1996 OR subaru-corsica-1998 OR subaru-xgames-2006\n"
"  xsara-turkey-2006, fabia-redbull, focus-norway-2007,\n"
"  focus-montecarlo-2006, 307-montecarlo-2006, lancer-finland-2001\n"
"HQ Cars/CMR2.5/V6 automatic import not supported; assets not bundled.\n"
"Scenery preparation requires supported original archives plus Python3.10+\n"
"with Pillow/numpy, or uv. Existing converter .venv is reused. Without the\n"
"explicit install flag no dependency or Python download is allowed. Generated\n"
"packs are private, hash-checked, versioned and never overwrite old packs.\n"
"Unspecified scenery kind stays original when generating a new pack.\n"
"No flags: existing settings/sidecars unchanged. Original means this launch\n"
"only. Explicit mode flags override settings without rewriting the INI.\n";}
#endif
