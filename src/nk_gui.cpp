#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#define NK_GDI_IMPLEMENTATION
#include "nk_gui.h"

#include "nuklear/nuklear.h"
#include "nuklear/nuklear_gdi.h"
#include "window.h"
#include "filehandling.h"
#include <codecvt>
#include <locale>
#include <cstring>

GdiFont* font;
struct nk_context *ctx;
unsigned int windowWidth;
unsigned int windowHeight;

// was a file loaded?
bool fileOpen=false;
// character number chosen through the combobox
int selectedPack;
// series names displayed in the series combobox
std::vector<std::string> seriesStrings;
// informative string displayed in the log at the bottom
std::string infoString;
// strings edited by the textboxes
std::string editStringPackNameID;
std::string editStringPackName;

void SetupGui(WINDOW_DATA &windowData,unsigned int initialWindowWidth, unsigned int initialWindowHeight){
    font = nk_gdifont_create("Segoe UI", 18);
    windowWidth=initialWindowWidth;
    windowHeight=initialWindowHeight;
    ctx = nk_gdi_init(font, windowData.dc, initialWindowWidth, initialWindowHeight);
    
    selectedPack=0;
    seriesStrings.push_back("DM");
    seriesStrings.push_back("GX");
    seriesStrings.push_back("5DS");
    seriesStrings.push_back("ZEXAL");
    seriesStrings.push_back("ARC-V");
    seriesStrings.push_back("VRAINS");
    seriesStrings.push_back("NONE");
    
    infoString="No file loaded. Open a packdefdata_X.bin file to begin.";
    
}

void HandleInput(WINDOW_DATA &windowData){
        MSG msg;
        nk_input_begin(ctx);
        if (windowData.needs_refresh == 0) {
            if (GetMessageW(&msg, NULL, 0, 0) <= 0)
                windowData.running = 0;
            else {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            windowData.needs_refresh = 1;
        } else windowData.needs_refresh = 0;

        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT)
                windowData.running = 0;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            windowData.needs_refresh = 1;
        }
        nk_input_end(ctx);
        
}

int HandleEvent(const EVENT_DATA &eventData){
    switch (eventData.msg)
    {
	case WM_SIZE:
        windowWidth=LOWORD(eventData.lparam);
        windowHeight=HIWORD(eventData.lparam);
		return nk_gdi_handle_event(eventData.wnd, eventData.msg, eventData.wparam, eventData.lparam);
    default:
        return nk_gdi_handle_event(eventData.wnd, eventData.msg, eventData.wparam, eventData.lparam);
    }
    
}

void HandleGui(FILE_DATA &fileData){
    if (nk_begin(ctx, "Demo", nk_rect(0, 0, windowWidth, windowHeight),
        0))
    {
        nk_layout_row_static(ctx, 0, 100, 2);
        if (nk_menu_begin_label(ctx,"FILE",NK_TEXT_LEFT,nk_vec2(100, 100))){
            nk_layout_row_dynamic(ctx, 0, 1);
            if(nk_menu_item_label(ctx, "OPEN", NK_TEXT_LEFT)){
                std::string filename=OpenFilename("YGO LOTD LE Pack Definition Data Files (*.bin)\0*.*\0");
                if (!filename.empty()){
                    bool success=ReadFile(filename,fileData);
                    if (success){
                        fileOpen=true;
                        selectedPack=0;
                        UpdateTextEdits(selectedPack,fileData);
                        infoString=std::to_string(fileData.packCount)+" packs loaded from file "+filename;
                    }
                    else{
                        infoString="Either the file could not be opened, or there was a read error.";
                    }
                }
            }
            if(nk_menu_item_label(ctx, "SAVE", NK_TEXT_LEFT) && fileOpen){
                std::string filename=SaveFilename("YGO LOTD LE Pack Definition Data Files (*.bin)\0*.*\0");
                if (!filename.empty()){
                    UpdateStrings(selectedPack,fileData);
                    bool success=SaveFile(filename,fileData);
                    if (success){
                        infoString="Successfully saved data to "+filename;
                    }else{
                        infoString="Either the file could not be created, or there was a write error.";
                    }

                }
            }
            nk_menu_end(ctx);
        }
        if (nk_menu_begin_label(ctx,"EDIT",NK_TEXT_LEFT,nk_vec2(100, 100))){
            nk_layout_row_dynamic(ctx, 0, 1);
            if(nk_menu_item_label(ctx, "Add Pack Slot", NK_TEXT_LEFT) && fileOpen){
                UpdateStrings(selectedPack,fileData);
                fileData.packCount++;
                selectedPack=fileData.packCount-1;
                fileData.field1.push_back(fileData.field1[selectedPack-1]+1);
                fileData.field2.push_back(0);
                fileData.field3.push_back(400);
                fileData.field4.push_back(0x52);
                fileData.string1.push_back("Pack"+std::to_string(fileData.field1[selectedPack]));
                fileData.string2.push_back("Pack "+std::to_string(fileData.field1[selectedPack]));
                fileData.string3.push_back(u"");
                UpdateTextEdits(selectedPack,fileData);
            }
            nk_menu_end(ctx);
        }
        
        float ratio[]={0.2,0.6,0.2};
        nk_layout_row(ctx, NK_DYNAMIC, 0, 3, ratio);
        
        if (nk_button_label(ctx, "Prev") && fileOpen){
            if (selectedPack>0){
                UpdateStrings(selectedPack,fileData);
                selectedPack--;
                UpdateTextEdits(selectedPack,fileData);
            }
        }

        int comboWasClicked;
        std::string comboLabel;
        if (fileOpen)
            comboLabel=std::to_string(fileData.field1[selectedPack])+" - "+fileData.string1[selectedPack];
        if(nk_combo_begin_label(ctx, comboLabel.c_str(),nk_vec2(237,190),&comboWasClicked)){
            if (comboWasClicked){
                nk_popup_set_scroll(ctx,0,selectedPack*(ctx->current->layout->row.min_height+ctx->style.window.spacing.y));
            }
            if (fileOpen){
                nk_layout_row_dynamic(ctx, 0, 1);
                for (Long i=0; i< fileData.packCount; i++){
                    int selected=nk_false;
                    if (nk_selectable_label(ctx,(std::to_string(fileData.field1[i])+" - "+fileData.string1[i]).c_str(),NK_TEXT_LEFT,&selected)){
                        if (selectedPack!=i){
                            UpdateStrings(selectedPack,fileData);
                            selectedPack=i;
                            UpdateTextEdits(selectedPack,fileData);
                        }
                        nk_combo_close(ctx);
                    }
                }
            }
            nk_combo_end(ctx);
        }

        if (nk_button_label(ctx, "Next") && fileOpen){
            if (selectedPack+1<fileData.packCount){
                UpdateStrings(selectedPack,fileData);
                selectedPack++;
                UpdateTextEdits(selectedPack,fileData);
            }
        }
        nk_layout_row_dynamic(ctx, 200, 3);

        if(nk_group_begin_titled(ctx, "group_fields", "Fields", NK_WINDOW_TITLE|NK_WINDOW_BORDER)){
            nk_layout_row_dynamic(ctx, 0, 1);
            nk_label(ctx,"Pack ID",NK_TEXT_LEFT);
            nk_label(ctx,"Series ID",NK_TEXT_LEFT);
            nk_label(ctx,"Price",NK_TEXT_LEFT);
            nk_label(ctx,"Unknown Value",NK_TEXT_LEFT);
            nk_group_end(ctx);
        }
        
        if(nk_group_begin_titled(ctx, "group_hex", "Hex", NK_WINDOW_TITLE|NK_WINDOW_BORDER)){
            if (fileOpen){
                nk_layout_row_dynamic(ctx, 0, 1);
                nk_label(ctx,IntToHexString(fileData.field1[selectedPack]).c_str(),NK_TEXT_LEFT);
                nk_label(ctx,IntToHexString(fileData.field2[selectedPack]).c_str(),NK_TEXT_LEFT);
                nk_label(ctx,IntToHexString(fileData.field3[selectedPack]).c_str(),NK_TEXT_LEFT);
                nk_label(ctx,IntToHexString(fileData.field4[selectedPack]).c_str(),NK_TEXT_LEFT);
            }
            nk_group_end(ctx);
        }

        if(nk_group_begin_titled(ctx, "group_edit", "Edit", NK_WINDOW_TITLE|NK_WINDOW_BORDER)){
            if (fileOpen){
                nk_layout_row_dynamic(ctx, 0, 1);
                nk_label(ctx,std::to_string((int)(fileData.field1[selectedPack])).c_str(),NK_TEXT_LEFT);
                int seriesSelection=fileData.field2[selectedPack];
                if (seriesSelection<0 || seriesSelection >=seriesStrings.size())
                    seriesSelection=seriesStrings.size()-1;
                if(nk_combo_begin_label(ctx, seriesStrings[seriesSelection].c_str(),nk_vec2(111,265))){
                    nk_layout_row_dynamic(ctx, 0, 1);
                    for (Long i=0; i< seriesStrings.size(); i++){
                        int selected=nk_false;
                        if (nk_selectable_label(ctx,seriesStrings[i].c_str(),NK_TEXT_LEFT,&selected)){
                            seriesSelection=i;
                            if (seriesSelection>=seriesStrings.size()-1)
                                fileData.field2[selectedPack]=0xFFFFFFFF;
                            else
                                fileData.field2[selectedPack]=i;
                            nk_combo_close(ctx);
                            
                        }
                    }
                    nk_combo_end(ctx);
                }
                
                nk_property_int(ctx,"#",0,reinterpret_cast<int*>(&(fileData.field3[selectedPack])),INT_MAX,1,1.0);
                nk_property_int(ctx,"#",0,reinterpret_cast<int*>(&(fileData.field4[selectedPack])),INT_MAX,1,1.0);
            }
            nk_group_end(ctx);
        }

        nk_layout_row_dynamic(ctx, 160, 1);
        if(nk_group_begin_titled(ctx, "group_strings", "Strings", NK_WINDOW_TITLE|NK_WINDOW_BORDER)){
            if (fileOpen){
                float ratio[]={0.22,0.78};
                nk_layout_row(ctx, NK_DYNAMIC, 0, 2, ratio);
                nk_label(ctx,"ID String: ",NK_TEXT_LEFT);
                nk_edit_string_zero_terminated(ctx,NK_EDIT_FIELD|NK_EDIT_SELECTABLE ,const_cast<char*>(editStringPackNameID.c_str()),52,0);
                nk_label(ctx,"Name: ",NK_TEXT_LEFT);
                nk_edit_string_zero_terminated(ctx,NK_EDIT_FIELD|NK_EDIT_SELECTABLE ,const_cast<char*>(editStringPackName.c_str()),52,0);
                std::string u8_conv = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.to_bytes(fileData.string3[selectedPack]);
                nk_label(ctx,"Unused String: ",NK_TEXT_LEFT);
                nk_label(ctx,u8_conv.c_str(),NK_TEXT_LEFT);
            }
            nk_group_end(ctx);
        }
        nk_layout_row_dynamic(ctx,0,1);
        nk_edit_string_zero_terminated(ctx,NK_EDIT_INACTIVE,const_cast<char*>(("INFO: "+infoString).c_str()),INT_MAX,nk_filter_default);
    }
    nk_end(ctx);
}

void UpdateStrings(int idx, FILE_DATA &fileData){
    fileData.string1[idx]=editStringPackNameID;
    fileData.string1[idx].resize(strlen(&editStringPackNameID[0]));
    fileData.string2[idx]=editStringPackName;
    fileData.string2[idx].resize(strlen(&editStringPackName[0]));
}

void UpdateTextEdits(int idx, FILE_DATA &fileData){
    editStringPackNameID=fileData.string1[idx];
    editStringPackNameID.resize(52);
    editStringPackName=fileData.string2[idx];
    editStringPackName.resize(52);
}


void RenderGui(){
    nk_gdi_render(nk_rgb(30,30,30));
}

void CleanupGui(){
    nk_gdifont_del(font);
}

void UpdateWindowSize(unsigned int width, unsigned int height){
    windowWidth=width;
    windowHeight=height;
}