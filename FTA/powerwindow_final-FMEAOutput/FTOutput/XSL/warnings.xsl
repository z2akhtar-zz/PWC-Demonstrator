<?xml version="1.0"?>
<xsl:stylesheet version="1.0"
      xmlns:faultTree ="http://www.hiphops.com/namespace/faultTree"
	  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"

>
<xsl:template match="HiP-HOPS_Results">
    <xsl:apply-templates select="WarningList" />
  </xsl:template>

<xsl:template match="WarningList">
<TABLE id="ContentTable" cellspacing="0">
      <TR>
        <TD>
<TABLE id="CutSetTable" cellSpacing="0">
<xsl:for-each select="Warning">
	<TR class="DataRow">
											<TD>
											<xsl:value-of select="."/>
											</TD>
											</TR>
	</xsl:for-each>
</TABLE>
</TD>
											</TR>
											</TABLE>
 </xsl:template>
</xsl:stylesheet>