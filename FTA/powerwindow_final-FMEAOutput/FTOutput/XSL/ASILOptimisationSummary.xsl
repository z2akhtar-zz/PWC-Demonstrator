<?xml version="1.0"?>



<xsl:stylesheet version="1.0"
      xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:param name="IndividualIDParam"/>
  <xsl:template match="/">
    <xsl:apply-templates select="CoitGA"></xsl:apply-templates>
    <xsl:apply-templates select="HiP-HOPS_Results/ExhaustiveSafetyAllocations"></xsl:apply-templates>
    <xsl:apply-templates select="NSGA"></xsl:apply-templates>


  </xsl:template>
<xsl:template match="NSGA">
  <p><a href="javascript:void(0)" onclick="window.location.reload()">Back to List</a></p>
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
								Number of Generations Without Improvement Allowed:
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
        Run Number
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

  <xsl:template match="CoitGA">
<p><a href="javascript:void(0)" onclick="window.location.reload()">Back to List</a></p>
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
          Number of Generations Without Improvement Allowed:
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

  <xsl:template match="HiP-HOPS_Results/ExhaustiveSafetyAllocations">
<p><a href="javascript:void(0)" onclick="window.location.reload()">Back to List</a></p>
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
        Run Number
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