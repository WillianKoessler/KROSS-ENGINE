#include "Panel.h"
#include "Kross.h"

namespace Kross {
	Panel::PanelManager Panel::s_Manager = Panel::PanelManager();
	Panel::AppSettings Panel::s_AppManager = Panel::AppSettings();

	void Panel::ShowHelperMarker(const std::string& msg, float size)
	{
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * size);
			ImGui::TextUnformatted(msg.c_str());
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}
	void Kross::Panel::MessageBoxDialog(MessageBoxSpecs& specs)
	{
		if (specs.show) {
			if (!specs.id) {
				specs.id = "Invalid Name for PopUp";
				specs.func = []() {ImGui::Text("The 'MessageBoxDialog was called with invalid (bool *show)."); };
				specs.buttontype = OK;
			}

			ImGui::OpenPopup(specs.id);
			if (ImGui::BeginPopupModal(specs.id, NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				specs.func();

				switch (specs.buttontype) {
					default:
						if (ImGui::Button("OK", ImVec2(50, 0))) {
							ImGui::CloseCurrentPopup();
							specs.show = false;
						}
					case OK:
						if (ImGui::Button("OK", ImVec2(50, 0))) {
							ImGui::CloseCurrentPopup();
							specs.show = false;
						}
				}
				ImGui::EndPopup();
			}
		}
	}
}
//if (id == "Style Open File")
//{
//	//char buff[256] = { 0 };
//	//ImGui::SetWindowFocus();
//	//////if (!ImGui::IsAnyItemActive())
//	//////	ImGui::SetKeyboardFocusHere(0);
//	//////if (ImGui::InputText("file", buff, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {///*Disabled till i know how to use InputText() to write into a buffer*/ || ImGui::Button("OK", ImVec2(50, 0))) {
//	////	//ImGui::GetStyle() = LoadStyle(buff[0] ? buff : "imguiStyle.ini");
//	//ImGui::GetStyle() = LoadStyle("imguiStyle.ini");
//	//ImGui::CloseCurrentPopup();
//	//*show = false;
//	////}
//	////ImGui::SetItemDefaultFocus();
//	////ImGui::SameLine();
//	////if (ImGui::Button("Cancel", ImVec2(50, 0))) {
//	////	ImGui::CloseCurrentPopup();
//	////	*show = false;
//	////}
//} else if (id == "Error: 20")
//{
//	ImGui::Text("This feature has not been implemented yet.");
//	auto w = ImGui::GetWindowSize().x;
//	ImGui::PushItemWidth(-w / 2);
//	if (ImGui::Button("Cancel"))
//	{
//		ImGui::CloseCurrentPopup();
//		*show = false;
//	}
//	ImGui::PopItemWidth();
//} else
//{
//	ImGui::Text("An Error has ocurred!\nThe ID for this PopUp Window was not setted.");
//	if (ImGui::Button("Cancel"))
//	{
//		ImGui::CloseCurrentPopup();
//		*show = false;
//	}
//}