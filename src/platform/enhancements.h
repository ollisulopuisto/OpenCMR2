// Optional enhancements are per launch. Never save selections into the profile.
#ifndef OPENCMR2_ENHANCEMENTS_H
#define OPENCMR2_ENHANCEMENTS_H
#include <string>
#include <map>
struct EnhancementOptions {
    std::string trees, spectators, names, cars, modsDir, carNames, roads, textures;
    bool installDependencies=false,prepareOnly=false;
    std::map<std::string,int> overrides;
    bool Consume(int &i,int argc,char **argv,std::string &error) {
        std::string key=argv[i];
        auto equal=key.find('=');
        std::string inlineValue;
        if(equal!=key.npos){inlineValue=key.substr(equal+1);key.resize(equal);}
        if(equal==key.npos && key=="--prepare-only"){prepareOnly=true;return true;}
        if(equal==key.npos && key=="--install-enhancement-deps"){installDependencies=true;return true;}
        if(key!="--textures"&&key!="--roads"&&key!="--tree-wind"&&key!="--car-names"&&key!="--trees"&&key!="--spectators"&&key!="--driver-names"&&key!="--cars"&&key!="--mods-dir"&&key!="--effect")return false;
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
            static const char *allowed[]={"dust_plume","body_dirt","dust_volume","volumetric_clouds","spatial_aa","metalfx_spatial","motion_blur","bloom","particles","shadows","depth_of_field","god_rays","color_grading"};
            bool known=false;for(auto key:allowed)if(name==key)known=true;
            if(!known||(v!="on"&&v!="off"))error="--effect expects a listed NAME=on/off";
            else overrides["video."+name]=v=="on";
        }
        if(key=="--cars"){if(value.empty())error="--cars needs a nonempty alternative list";else cars=value;}
        if(key=="--mods-dir"){if(value.empty())error="--mods-dir needs a folder";else modsDir=value;}

        return true;
    }
    void Apply(){
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
"                              depth_of_field,god_rays,color_grading\n"
"                              Requested is not proof of backend support;\n"
"                              see runtime log. Temporal reconstruction absent.\n"
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
