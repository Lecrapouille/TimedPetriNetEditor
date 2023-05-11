//------------------------------------------------------------------------------
bool PetriNet::exportToCodesys(std::string const& filename) const
{
  const size_t RECEP = 10000u;
  const size_t TRANS = 20000u;
  const size_t ACTIO = 30000u;
  const size_t STEPS = 40000u;
  const size_t BRANC = 50000u;

    // Check if file exists
    std::ofstream file(filename);
    if (!file)
    {
        m_message.str("");
        m_message << "Failed to export the Petri net to '" << filename
                  << "'. Reason was " << strerror(errno) << std::endl;
        return false;
    }

    //TODO
    // Hash table to convert into localId
    //std::map<intptr_t, size_t> local_ids;
    //for (auto const& p: m_places)

    // Date
    char buffer[32];
    time_t current_time = ::time(nullptr);
    strftime(buffer, sizeof (buffer), "%Y-%M-%DT%H:%M:%S.0", localtime(&current_time));

    // Header
    file << R"PN(<?xml version="1.0" encoding="utf-8"?>
<project xmlns="http://www.plcopen.org/xml/tc6_0200">
)PN";

    file << "  <fileHeader companyName=\"\" productName=\"CODESYS\" productVersion=\"CODESYS V3.5\" creationDateTime=\"" << buffer << "\" />" << std::endl;
    file << "  <contentHeader name=\"" << name() << ".project\" modificationDateTime=\"" << buffer << "\">" << std::endl;
    file << R"PN(    <coordinateInfo>
      <fbd><scaling x="1" y="1" /></fbd>
      <ld><scaling x="1" y="1" /></ld>
      <sfc><scaling x="1" y="1" /></sfc>
    </coordinateInfo>
    <addData>
      <data name="http://www.3s-software.com/plcopenxml/projectinformation" handleUnknown="implementation">
        <ProjectInformation />
      </data>
    </addData>
  </contentHeader>
)PN";

  struct VAR { std::string name = "Trans0"; std::string type = "BOOL"; std::string value = "FALSE"; std::string doc = "MON commentaire"; };
  std::vector<VAR> sensors;

    // SFC begin
    file << "  <types>\n    <dataTypes />\n    <pous>" << std::endl;
    file << "      <pou name=\"PLC_PRG\" pouType=\"program\">" << std::endl;
    file << "        <interface>\n          <localVars>" << std::endl;
    for (auto const& it: sensors)
    {
      file << "            <variable name=\"" << it.name << "\">" << std::endl;
      file << "              <type><" << it.type << " /></type>" << std::endl;
      file << "              <initialValue><simpleValue value=\"" << it.value << "\" /></initialValue>" << std::endl;
      if (!it.doc.empty())
      {
        file << "              <documentation><xhtml xmlns=\"http://www.w3.org/1999/xhtml\">" << it.doc << "</xhtml></documentation>" << std::endl;
      }
      file << "            </variable>" << std::endl;
    }

    file << "          </localVars>\n        </interface>" << std::endl;
    file << "        <body>\n          <SFC>" << std::endl;

    // Toutes les actions
    // Toutes les receptivites
    // Toutes les transitions
    // Toutes les branches divergence en OU

    for (auto const& p: m_places)
    {
        // Actions

        // Step
        file << "            <step localId=\"" << p.key << "\" initialStep=\"" << (p.tokens != 0u) << "\" name=\"" << p.caption << "\">";
        file << "              <position x=\"0\" y=\"0\" />" << std::endl;
        file << "              <connectionPointIn />" << std::endl;
        file << "              <connectionPointOut formalParameter=\"sfc\" />" << std::endl;
        file << "              <addData>" << std::endl;
        file << "              </addData>" << std::endl;
        file << "            </step>" << std::endl;
    }




    for (auto const& t: m_transitions)
    {
        // Receptivite
        file << "            <inVariable localId=\"" << t.key << "_recept" << "\">" << std::endl;
        file << "              <position x=\"0\" y=\"0\" />" << std::endl;
        file << "              <connectionPointOut />" << std::endl;
        file << "              <expression>" << (t.caption.empty() ? "TRUE" : t.caption) << "</expression>" << std::endl;
        file << "            </inVariable>" << std::endl;

        // Transition
        file << "            <transition localId=\"8\">" << std::endl;
        file << "              <position x=\"0\" y=\"0\" />" << std::endl;
        file << "              <connectionPointIn>" << std::endl;
        file << "                <connection refLocalId=\"4\" formalParameter=\"sfc\" />" << std::endl;
        file << "              </connectionPointIn>" << std::endl;
        file << "              <condition>" << std::endl;
        file << "                <connectionPointIn>" << std::endl;
        file << "                  <connection refLocalId=\"7\" />" << std::endl;
        file << "                </connectionPointIn>" << std::endl;
        file << "              </condition>" << std::endl;
        file << "            </transition>" << std::endl;
    }

    // SFC end
    file << R"PN(
          </SFC>
        </body>
)PN";

    // data
    file << R"PN(
        <addData>
          <data name="http://www.3s-software.com/plcopenxml/sfcsettings" handleUnknown="implementation">
            <SFCSettings>

            </SFCSettings>
          </data>
          <data name="http://www.3s-software.com/plcopenxml/objectid" handleUnknown="discard">
            <ObjectId>0dbb3829-3eb0-41a6-a673-b66fe6b73677</ObjectId>
          </data>
        </addData>
      </pou>
    </pous>
  </types>
)PN";

    // Footer
    file << R"PN(
  <instances>
    <configurations />
  </instances>
  <addData>
    <data name="http://www.3s-software.com/plcopenxml/projectstructure" handleUnknown="discard">
      <ProjectStructure>
        <Object Name="PLC_PRG" ObjectId="0dbb3829-3eb0-41a6-a673-b66fe6b73677" />
      </ProjectStructure>
    </data>
  </addData>
</project>)PN";

    return true;
}
