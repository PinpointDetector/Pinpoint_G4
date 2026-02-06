void view_pinpoint(const char* filename = "pinpoint.gdml") {
    TGeoManager::Import(filename);
    gGeoManager->Print();
    TGeoVolume* top = gGeoManager->GetTopVolume();
    top->Draw("ogl");
}

