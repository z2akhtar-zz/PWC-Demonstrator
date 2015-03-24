<?xml version="1.0"?>



<xsl:stylesheet version="1.0"
      xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:param name="IndividualIDParam"/>
  <xsl:param name="XmlFileName"/>
  <xsl:template match="/">
    <xsl:apply-templates select="HiP-HOPS_Results/CoitGA"></xsl:apply-templates>
    <xsl:apply-templates select="HiP-HOPS_Results/SafetyAllocations"></xsl:apply-templates>
    <xsl:apply-templates select="HiP-HOPS_Results/NSGA"></xsl:apply-templates>


  </xsl:template>
<xsl:template match="HiP-HOPS_Results/NSGA">
<xsl:if test="$IndividualIDParam!=''">
<p><xsl:element name="a">
                        <xsl:attribute name="href">
                          javascript:
                        optimisationTransform('<xsl:value-of select="$XmlFileName" />','', '');
                        optimisationSummaryTransform('<xsl:value-of select="$XmlFileName" />');
                        </xsl:attribute>
                        Back to List
                      </xsl:element></p>
					  </xsl:if>
<TABLE id="TopEventDataTable" cellSpacing="1">
						<TR>
							<TD class="TopEventsLabel">
								Model File Name:
							</TD>
							<TD class="TopEventsData">
								<xsl:value-of select="ModelFileName" />
							</TD>
						</TR>
						<TR>
							<TD class="TopEventsLabel">
								Number of Generations Run:
							</TD>
							<TD class="TopEventsData">
								<xsl:value-of select="Generation" />
							</TD>
						</TR>
  <xsl:choose>
    <xsl:when test="$IndividualIDParam!=''">
      <xsl:apply-templates select="ArchiveNonDominatedPopulations/NonDominatedPopulation/Individual[./IndividualID=$IndividualIDParam]"/>
      
    </xsl:when>
  </xsl:choose>
					</TABLE>
</xsl:template>

  <xsl:template match="Individual">
    <TR>
      <TD class="TopEventsLabel">
        Individual ID
      </TD>
      <TD class="TopEventsData">
        <xsl:value-of select="IndividualID"/>
      </TD>
    </TR>
    <xsl:apply-templates select="Evaluations/Evaluation"/>

  </xsl:template>

  <xsl:template match="Evaluation">
    <TR>
      <TD class="TopEventsLabel">
        <xsl:value-of select="Name"/>
      </TD>
      <TD class="TopEventsData">
        <xsl:value-of select="Value" />
      </TD>
    </TR>
    
  </xsl:template>

  <xsl:template match="HiP-HOPS_Results/CoitGA">
  <xsl:if test="$IndividualIDParam!=''">
<p><xsl:element name="a">
                        <xsl:attribute name="href">
                          javascript:
                        optimisationTransform('<xsl:value-of select="$XmlFileName" />','', '');
                        optimisationSummaryTransform('<xsl:value-of select="$XmlFileName" />');
                        </xsl:attribute>
                        Back to List
                      </xsl:element></p>
					  </xsl:if>
    <TABLE id="TopEventDataTable" cellSpacing="1">
      <TR>
        <TD class="TopEventsLabel">
          Model File Name:
        </TD>
        <TD class="TopEventsData">
          <xsl:value-of select="ModelFileName" />
        </TD>
      </TR>
      <TR>
        <TD class="TopEventsLabel">
          Number of Generations Run:
        </TD>
        <TD class="TopEventsData">
          <xsl:value-of select="Generation" />
        </TD>
      </TR>
      <xsl:choose>
        <xsl:when test="$IndividualIDParam!=''">
          <xsl:apply-templates select="Population/Individual[./IndividualID=$IndividualIDParam]"/>

        </xsl:when>
      </xsl:choose>
    </TABLE>
  </xsl:template>

  <xsl:template match="HiP-HOPS_Results/SafetyAllocations">
  <xsl:if test="$IndividualIDParam!=''">
<p><xsl:element name="a">
                        <xsl:attribute name="href">
                          javascript:
                        optimisationTransform('<xsl:value-of select="$XmlFileName" />','', '');
                        optimisationSummaryTransform('<xsl:value-of select="$XmlFileName" />');
                        </xsl:attribute>
                        Back to List
                      </xsl:element></p>
					  </xsl:if>
    <TABLE id="TopEventDataTable" cellSpacing="1">
      <TR>
        <TD class="TopEventsLabel">
          Model File Name:
        </TD>
        <TD class="TopEventsData">
          <xsl:value-of select="ModelFileName" />
        </TD>
      </TR>
     
      <xsl:choose>
        <xsl:when test="$IndividualIDParam!=''">
          <xsl:apply-templates select="Population/Individual[./IndividualID=$IndividualIDParam]"/>

        </xsl:when>
      </xsl:choose>
    </TABLE>
  </xsl:template>

  <xsl:template match="Individual">
    <TR>
      <TD class="TopEventsLabel">
        Individual ID
      </TD>
      <TD class="TopEventsData">
        <xsl:value-of select="IndividualID"/>
      </TD>
    </TR>
    <xsl:apply-templates select="Evaluations/Evaluation"/>

  </xsl:template>

  <xsl:template match="Evaluation">
    <TR>
      <TD class="TopEventsLabel">
        <xsl:value-of select="Name"/>
      </TD>
      <TD class="TopEventsData">
        <xsl:value-of select="Value" />
      </TD>
    </TR>

  </xsl:template>
</xsl:stylesheet>