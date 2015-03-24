<?xml version="1.0"?>
<xsl:stylesheet version="1.0"
      xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:param name="XmlFileName"/>
  <xsl:param name="Title"/>
  <xsl:template match="HiP-HOPS_Results">
    <TABLE id="MenuTable">
<TR class="MenuRow">
	<TD>
		<DIV id="hipHop">
			<img src="FTOutput/Images/hiphopslogo.gif" width="62px" height="50px" />
		</DIV>
    <DIV id="MenuTitle">
      <xsl:value-of select="$Title"/>
    </DIV>
	</TD>
	<xsl:variable name="build" select="../@build" />
	<xsl:if test="contains($build, 'SimulationX') and string-length($build) = string-length('SimulationX')">
                  <TD id="logosCell">
          <TABLE id="logosTable">
            <TR>
              <TD>
                <DIV id="logos">
                  <img src="FTOutput/images/logos.png" width="320px" height="40px" />
                </DIV>
              </TD>

            </TR>
          </TABLE>
        </TD>
    </xsl:if>
</TR>
<TR class="MenuRow">
<TD>
					<DIV id="divMenuBar" >
						<TABLE id="tblMenuBar">
							<TR>
	<xsl:apply-templates select="FaultTrees" />
	<xsl:apply-templates select="CoitGA" />
	<xsl:apply-templates select="NSGA" />
	<xsl:apply-templates select="SafetyAllocations" />
	<xsl:apply-templates select="WarningList" />
	</TR>

</TABLE>
</DIV>
</TD>
</TR>

</TABLE>
  </xsl:template>
  <xsl:template match="WarningList">
  <xsl:if test="((/HiP-HOPS_Results/FaultTrees) and count(/HiP-HOPS_Results/FaultTrees/FaultTree) > 0) or (/HiP-HOPS_Results/FMEA) or (/HiP-HOPS_Results/SafetyAllocations) or (/HiP-HOPS_Results/CoitGA)">
                    <TD>
                      |
                    </TD>
                  </xsl:if>
                  <TD CLASS="clsMenuBarItem" id="tdMenuBarItemWarnings">

                    <xsl:element name="a">
                      <xsl:attribute name="href">
                       javascript:
                        warningsTransform('<xsl:value-of select="$XmlFileName" />');
                        menuTransform('FaultTrees.xml','Analysis Warnings');
                      </xsl:attribute>

                      Warnings
                    </xsl:element>
                  </TD>
  </xsl:template>
  <xsl:template match="SafetyAllocations">
  <xsl:if test="((/HiP-HOPS_Results/FaultTrees) and count(/HiP-HOPS_Results/FaultTrees/FaultTree) > 0) or (/HiP-HOPS_Results/FMEA)">
                    <TD>
                      |
                    </TD>
                  </xsl:if>
  <TD CLASS="clsMenuBarItem" id="tdMenuBarItemWarnings">

                  <xsl:element name="a">
                    <xsl:attribute name="href">
                      javascript:
                        optimisationTransform('<xsl:value-of select="$XmlFileName" />','', '');
                        optimisationSummaryTransform('<xsl:value-of select="$XmlFileName" />');
                        //changeTitle('HiPHOPS Safety Allocations');
                    </xsl:attribute>

                    Safety Allocations
                  </xsl:element>
                </TD>
  </xsl:template>
  <xsl:template match="NSGA">
  <xsl:if test="((/HiP-HOPS_Results/FaultTrees) and count(/HiP-HOPS_Results/FaultTrees/FaultTree) > 0) or (/HiP-HOPS_Results/FMEA)">
                    <TD>
                      |
                    </TD>
                  </xsl:if>
  <TD CLASS="clsMenuBarItem" id="tdMenuBarItemWarnings">

                  <xsl:element name="a">
                    <xsl:attribute name="href">
                      javascript:
                        optimisationTransform('<xsl:value-of select="$XmlFileName" />','', '');
                        optimisationSummaryTransform('<xsl:value-of select="$XmlFileName" />');
                        //changeTitle('HiPHOPS Safety Allocations');
                    </xsl:attribute>

                    Optimisation
                  </xsl:element>
                </TD>
							
  </xsl:template>
  <xsl:template match="CoitGA">
  <xsl:if test="((/HiP-HOPS_Results/FaultTrees) and count(/HiP-HOPS_Results/FaultTrees/FaultTree) > 0) or (/HiP-HOPS_Results/FMEA)">
                    <TD>
                      |
                    </TD>
                  </xsl:if>
  <TD CLASS="clsMenuBarItem" id="tdMenuBarItemWarnings">

                  <xsl:element name="a">
                    <xsl:attribute name="href">
                      javascript:
                        optimisationTransform('<xsl:value-of select="$XmlFileName" />','', '');
                        optimisationSummaryTransform('<xsl:value-of select="$XmlFileName" />');
                        //changeTitle('HiPHOPS Safety Allocations');
                    </xsl:attribute>

                    Optimisation
                  </xsl:element>
                </TD>
							
  </xsl:template>
<xsl:template match="FaultTrees">
<xsl:variable name="maxOrder">
      <xsl:call-template name="max">
        <xsl:with-param name="list" select="FaultTree/AllCutSets/CutSets/@order"></xsl:with-param>
      </xsl:call-template>
    </xsl:variable>
<xsl:variable name="minOrder">
      <xsl:call-template name="min">
        <xsl:with-param name="list" select="FaultTree/AllCutSets/CutSets/@order"></xsl:with-param>
		<xsl:with-param name="maxValue" select="$maxOrder"></xsl:with-param>
      </xsl:call-template>
    </xsl:variable>
                <xsl:if test="FaultTree">
								<TD CLASS="clsMenuBarItem" id="tdMenuBarItemTopEvents">
									
									<xsl:element name="a"> 
									<xsl:attribute name="href">
                    javascript:
                    overviewTransform('FaultTrees.xml', 'descending');
                    menuTransform('FaultTrees.xml','Fault Trees Overview');
                  </xsl:attribute> 
									
									FaultTrees
								</xsl:element> 
								</TD>
               
								
                </xsl:if>
                <xsl:if test="(/HiP-HOPS_Results/FaultTrees/FMEA)">
                  
                  <xsl:if test="(/HiP-HOPS_Results/FaultTrees) and count(/HiP-HOPS_Results/FaultTrees/FaultTree) > 0">
                    <TD>
                      |
                    </TD>
                  </xsl:if>
                  <TD CLASS="clsMenuBarItem" ID="tdMenuBarItemFMEA">
                    <xsl:element name="a">
                      <xsl:attribute name="href">
                        javascript:
                        fmeaTransform('FaultTrees.xml', 'DirectEffects', '0', '100');
                        menuTransform('FaultTrees.xml','Failure Modes and Effects Analysis');
                      </xsl:attribute>
                      FMEA <xsl:call-template name="min">
        <xsl:with-param name="list" select="FaultTree/AllCutSets/CutSets/@order"></xsl:with-param>
      </xsl:call-template>
                    </xsl:element>
                  </TD>
                </xsl:if>
        
                </xsl:template>

  <xsl:template match="FaultTree">
      <DIV>
        <xsl:variable name="name">
          <xsl:value-of select ="Name"/>
        </xsl:variable>
        <xsl:element name="a">
          <xsl:attribute name="href">
            javascript:
                topEventTransform('FaultTrees.xml','<xsl:value-of select="Name" />');
                faultTreesTransform('<xsl:value-of select="$XmlFileName" />','<xsl:value-of select="Name" />');
                menuTransform('FaultTrees.xml','Fault Tree View: <xsl:value-of select="Name" />');

          </xsl:attribute>
          <xsl:value-of select="Name" />
        </xsl:element>
      </DIV>
  </xsl:template>
  
  <xsl:template name ="min">
    <xsl:param name ="list" />
	<xsl:param name ="maxValue" />
    <xsl:choose >
      <xsl:when test ="$list">
        <xsl:variable name ="first" select ="$list[1]" />

        <xsl:variable name ="rest">
          <xsl:call-template name ="min">
            <xsl:with-param name ="list" select ="$list[position() != 1]" />
			<xsl:with-param name ="maxValue" select ="$maxValue" />
          </xsl:call-template>
        </xsl:variable>
		<xsl:choose>
          <xsl:when test="$first &lt; $rest">
            <xsl:value-of select ="$first"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select ="$rest"/>
          </xsl:otherwise>
        </xsl:choose>

      </xsl:when>
      <xsl:otherwise> <xsl:value-of select ="$maxValue"/></xsl:otherwise>

    </xsl:choose>

  </xsl:template>
  
  <xsl:template name ="max">
    <xsl:param name ="list" />
    <xsl:choose >
      <xsl:when test ="$list">
        <xsl:variable name ="first" select ="$list[1]" />

        <xsl:variable name ="rest">
          <xsl:call-template name ="max">
            <xsl:with-param name ="list" select ="$list[position() != 1]" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:choose>
          <xsl:when test="$first &gt; $rest">
            <xsl:value-of select ="$first"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select ="$rest"/>
          </xsl:otherwise>
        </xsl:choose>

      </xsl:when>
      <xsl:otherwise>0</xsl:otherwise>

    </xsl:choose>

  </xsl:template>
  
</xsl:stylesheet>