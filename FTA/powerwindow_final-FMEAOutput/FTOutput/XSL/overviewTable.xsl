<?xml version="1.0"?>
<xsl:stylesheet version="1.0"
      xmlns:faultTree ="http://www.hiphops.com/namespace/faultTree"
	  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"

>
  <xsl:param name="XmlFileName"/>
  <xsl:param name="sortOrder"/>
  <xsl:template match="HiP-HOPS_Results">
   <xsl:apply-templates select="FaultTrees" />
  </xsl:template>

 
  
  <xsl:template match="FaultTrees">
    <xsl:variable name="maxOrder">
      <xsl:call-template name="max">
        <xsl:with-param name="list" select="FaultTree/CutSetsSummary/CutSets/@order"></xsl:with-param>
      </xsl:call-template>
    </xsl:variable>
   
    
    <table id="FaultTreeOverviewTable" cellspacing="0">
      <tr class="HeaderRow">
        <td rowspan="3">Fault Trees</td>
        <xsl:if test="FaultTree/Unavailability">
          <td rowspan="3">Unavailability
		  (
			<xsl:choose>
			<xsl:when test="$sortOrder = 'ascending'">ascending</xsl:when>
			<xsl:otherwise>
			<xsl:element name="a">
              <xsl:attribute name="href">
                javascript:
                    overviewTransform('<xsl:value-of select="$XmlFileName" />', 'ascending');
                    menuTransform('<xsl:value-of select="$XmlFileName" />','Fault Trees Overview');
              </xsl:attribute>ascending</xsl:element>
			</xsl:otherwise>
			</xsl:choose>
			/
			<xsl:choose>
			<xsl:when test="$sortOrder = 'descending'">descending</xsl:when>
			<xsl:otherwise>
			<xsl:element name="a">
              <xsl:attribute name="href">
                javascript:
                    overviewTransform('<xsl:value-of select="$XmlFileName" />', 'descending');
                    menuTransform('<xsl:value-of select="$XmlFileName" />','Fault Trees Overview');
              </xsl:attribute>descending</xsl:element>
			</xsl:otherwise>
			</xsl:choose>
			)		  
		  </td>
        </xsl:if>
        <xsl:element name="td">
          <xsl:attribute name="colspan">
            <xsl:value-of select="$maxOrder +1"/>
          </xsl:attribute>
          Cut Sets
        </xsl:element>
      </tr>
      <tr class="HeaderRow">
        <td rowspan="2">Total</td>
        <xsl:element name="td">
          <xsl:attribute name="colspan">
            <xsl:value-of select="$maxOrder"/>
          </xsl:attribute>
          Order
        </xsl:element>
      </tr>
      <tr class="HeaderRow">
        
        
        <xsl:call-template name="generateCutSetOrderHeader" >
          <xsl:with-param name="maxOrder">
            <xsl:value-of select="$maxOrder"/>
          </xsl:with-param>
        </xsl:call-template>
      </tr>
      <xsl:for-each select="FaultTree">
	  <xsl:sort select="UnavailabilitySort" order="{$sortOrder}" data-type="number" />
        <tr>
          <td>
            <xsl:element name="a">
              <xsl:attribute name="href">
                javascript:
                topEventTransform('FaultTrees.xml','<xsl:value-of select="Name" />');
                faultTreesTransform('FaultTrees.xml','<xsl:value-of select="Name" />');
                menuTransform('FaultTrees.xml','Fault Tree View: <xsl:value-of select="Name" />');


              </xsl:attribute>
              <xsl:value-of select="Name"/>
            </xsl:element>
            
            
          </td>
          <xsl:if test="../FaultTree/Unavailability">
            
            <td>
              <xsl:choose>
                <xsl:when test ="Unavailability">
              <xsl:value-of select="Unavailability"/>
                </xsl:when>
                <xsl:otherwise>
                  0
                </xsl:otherwise>
              </xsl:choose>
            </td>
          </xsl:if>
          <td>
			<xsl:choose>
				<xsl:when test="CutSetsSummary/CutSets">
				<xsl:variable name="numberNonPrunedCutSets" select="sum(CutSetsSummary/CutSets[@pruned = 'false'])" />
				<xsl:variable name="totaNumberCutSets" select="sum(CutSetsSummary/CutSets)" />
				<xsl:element name="a">
				  <xsl:attribute name="href">
					javascript:
					topEventTransform('FaultTrees.xml','<xsl:value-of select="Name" />');
					cutSetsTransform('CutSets(<xsl:value-of select="@ID" />).xml','<xsl:value-of select="Name" />', 'unavailability', 'ascending','100', '0', '0','0');
					menuTransform('FaultTrees.xml','Cut Sets View: <xsl:value-of select="Name" />');
				  </xsl:attribute>
				  <xsl:value-of select="sum(CutSetsSummary/CutSets[@pruned = 'false'])"/><xsl:if test="$numberNonPrunedCutSets != $totaNumberCutSets"> (<xsl:value-of select="sum(CutSetsSummary/CutSets)"/>)</xsl:if>
				</xsl:element>
				</xsl:when>
				<xsl:otherwise>0</xsl:otherwise>
            </xsl:choose>
          </td>
          <xsl:call-template name="generateCutSetOrderColumns" >
            <xsl:with-param name="maxOrder">
              <xsl:value-of select="$maxOrder"/>
            </xsl:with-param>
          </xsl:call-template>
        </tr>
      </xsl:for-each>
    </table>

  </xsl:template>

	<!-- generateCutSetOrderHeader generates 1 to maxOrder column headers  -->
	<xsl:template name="generateCutSetOrderHeader">
		<!-- current order initialised to 1 -->
		<xsl:param name="currentOrder">1</xsl:param>
		<!-- maxOrder is the maximum size of cut set from all the trees -->
		<xsl:param name="maxOrder" />
		<!-- check we have not exceeded the maximum order -->
		<xsl:if test="not($currentOrder = $maxOrder +1)">
			<!-- only output if at least one fault tree has a cut set of current order -->
			<xsl:if test="FaultTree/CutSetsSummary/CutSets[@order = $currentOrder]" >
				<td>
					<xsl:value-of select="$currentOrder" />
				</td>
			</xsl:if>
			<!-- recursively call this template increasing the current order each time -->
			<xsl:call-template name="generateCutSetOrderHeader">
				<xsl:with-param name="currentOrder">
					<xsl:value-of select="$currentOrder + 1" />
				</xsl:with-param>
				<xsl:with-param name="maxOrder">
					<xsl:value-of select="$maxOrder" />
				</xsl:with-param>
			</xsl:call-template>
		</xsl:if>
	</xsl:template><!-- end of generateCutSetOrderHeader -->

	<!-- generateCutSetOrderColumns retrieves the number of cut sets of each order  -->
	<xsl:template name="generateCutSetOrderColumns">
		<!-- current order initialised to 1 -->
		<xsl:param name="currentOrder">1</xsl:param>
		<!-- maxOrder is the maximum size of cut set from all the trees -->
		<xsl:param name="maxOrder" />
		<!-- check we have not exceeded the maximum order -->
		<xsl:if test="not($currentOrder = $maxOrder +1)">
			<!-- only output if at least one fault tree has a cut set of current order -->
			<xsl:if test="../FaultTree/CutSetsSummary/CutSets[@order = $currentOrder]" >
				<td>
					<xsl:choose>
						<!-- if there are cut sets of the current order output the number in a hyperlink to those cut sets -->
						<xsl:when test="CutSetsSummary/CutSets[@order = $currentOrder]">
							<xsl:choose>
							<!-- if there are cut sets that have not been pruned  hyperlink to those cut sets -->
							<xsl:when test="CutSetsSummary/CutSets[@order = $currentOrder and @pruned = 'false']">
								<xsl:element name="a">
									<xsl:attribute name="href">
										javascript:
										javascript:
										topEventTransform('CutSets(<xsl:value-of select="@ID" />).xml','<xsl:value-of select="Name" />');
										cutSetsTransform('CutSets(<xsl:value-of select="@ID" />).xml','<xsl:value-of select="Name" />', 'unavailability', 'ascending','100', '0', '<xsl:value-of select="$currentOrder"/>','0');
										menuTransform('CutSets(<xsl:value-of select="@ID" />).xml','Cut Sets View: <xsl:value-of select="Name" />');
									</xsl:attribute>
									<xsl:value-of select="CutSetsSummary/CutSets[@order = $currentOrder]" />
								</xsl:element>
							</xsl:when>
							<!-- otherwise no hyperlink -->
							<xsl:otherwise><xsl:value-of select="CutSetsSummary/CutSets[@order = $currentOrder]" /></xsl:otherwise>
						</xsl:choose>
						</xsl:when>
						<!-- otherwise no hyperlink and the value is set to zero -->
						<xsl:otherwise>0</xsl:otherwise>
					</xsl:choose>
				</td>
			</xsl:if>
			<!-- recursively call this template increasing the current order each time -->
			<xsl:call-template name="generateCutSetOrderColumns">
				<xsl:with-param name="currentOrder">
					<xsl:value-of select="$currentOrder + 1" />
				</xsl:with-param>
				<xsl:with-param name="maxOrder">
					<xsl:value-of select="$maxOrder" />
				</xsl:with-param>
			</xsl:call-template>
		</xsl:if>
	</xsl:template> <!-- end of generateCutSetOrderColumns  -->
	<!-- max template returns the maximum value -->
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
	</xsl:template> <!-- end of max -->
</xsl:stylesheet>