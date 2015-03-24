<?xml version="1.0"?>
<xsl:stylesheet version="1.0"
      xmlns:faultTree ="http://www.hiphops.com/namespace/faultTree"
	  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"

>

<xsl:template match="SafetyAllocations">
  
  
<TABLE id="CutSetTable" cellSpacing="0">
  <TR id="AsilTitle">
    
    <xsl:element name="TD">
      <xsl:attribute name="colspan">
        <xsl:for-each select="SafetyAllocation">
          <xsl:if test="position() = 1">
            <xsl:value-of select="count(BasicEvent)"/>            
          </xsl:if>
        </xsl:for-each>
      </xsl:attribute>
      Safety Allocations
    </xsl:element>
  </TR>
  <TR class ="AsilHeaderRow">
    <xsl:for-each select="SafetyAllocation">
      <xsl:if test="position() = 1">
      <xsl:for-each select="BasicEvent">
        <TD>
           <xsl:value-of select="Name"/>
        </TD>      
      </xsl:for-each>
      </xsl:if>
    </xsl:for-each>
    
  </TR>
  
<xsl:for-each select="SafetyAllocation">
	<TR class="AsilDataRow">
    <xsl:for-each select="BasicEvent">
											<TD>
											<xsl:value-of select="SafetyLevel"/>
											</TD>
    </xsl:for-each>
											</TR>
	</xsl:for-each>
</TABLE>
 </xsl:template>
</xsl:stylesheet>