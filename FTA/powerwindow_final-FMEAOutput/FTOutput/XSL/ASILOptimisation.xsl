<?xml version="1.0"?>
<xsl:stylesheet version="1.0"
      xmlns:faultTree ="http://www.hiphops.com/namespace/faultTree"
	  xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:param name="xmlFile"/>
  <xsl:param name="sortType"/>
  <xsl:param name="sortOrder"/>
  <xsl:template match="/">
    <xsl:apply-templates select="CoitGA"></xsl:apply-templates>
    <xsl:apply-templates select="NSGA"></xsl:apply-templates>
    <xsl:apply-templates select="HiP-HOPS_Results/ExhaustiveSafetyAllocations"></xsl:apply-templates>
    
  </xsl:template>
  
<xsl:template match="NSGA">
  <TABLE id="ContentTable" cellSpacing="0">
			<TR>
			<TD>
        <xsl:choose>
					 <xsl:when test="ArchiveNonDominatedPopulations">
						<TABLE id="CutSetTable" cellSpacing="0">
							<TR class="HeaderRow">
								<xsl:for-each select="Objectives/Objective">
									<TD>
                    <xsl:element name="a">
                      <xsl:attribute name="href">
                        javascript:
                        var sortOrderParam = '<xsl:value-of select="$sortOrder" />';
                        var parameters = new Array();
                        var parameter = new Object();
                        parameter['name'] = 'xmlFile';
                        parameter['value'] = '<xsl:value-of select="$xmlFile"/>';
                        parameters.push(parameter);
                        
                        parameter = new Object();
                        parameter.name = 'sortType';
                        parameter.value = '<xsl:value-of select="Name" />';

                        parameters.push(parameter);
                        parameter = new Object();
                        parameter.name = 'sortOrder';
                        parameter.value = 'ascending';
                        if (sortOrderParam == 'ascending')
                        {
                        parameter.value = 'descending';
                        }
                        parameters.push(parameter);

                        runTransform('<xsl:value-of select="$xmlFile" />','FTOutput/XSL/ASILOptimisation.xsl', 'ContentSpace', parameters);

                      </xsl:attribute>
                      <xsl:value-of select="Name" />
                    </xsl:element>
									</TD>
								</xsl:for-each>	
								<TD>
									Configuration
								</TD>
							</TR>
							<xsl:for-each select="ArchiveNonDominatedPopulations/NonDominatedPopulation/Individual">
                <xsl:sort select="Evaluations/Evaluation[Name[.=$sortType]]/Value" order="{$sortOrder}" data-type="number"/>
								<TR class="DataRow">
									<xsl:for-each select="Evaluations/Evaluation">
										<TD>
										<xsl:value-of select="Value" />
									</TD>
									</xsl:for-each>
									<TD>
                    <xsl:element name="a">
                      <xsl:attribute name="href">
                        javascript:
                        var parameter = new Object();
                        parameter.name = 'IndividualIDParam';
                        parameter.value = '<xsl:value-of select="IndividualID" />';
                        var parameters = new Array();
                        parameters.push(parameter);
                        runTransform('<xsl:value-of select="$xmlFile" />','FTOutput/XSL/<xsl:value-of select="local-name(Encoding/*[1])" />.xsl', 'ContentSpace', parameters);
                        runTransform('<xsl:value-of select="$xmlFile" />','FTOutput/XSL/ASILOptimisationSummary.xsl', 'TopEventData', parameters);



                      </xsl:attribute>
                      Click here to see configuration
                    </xsl:element>
									</TD>
								</TR>
							</xsl:for-each>	
						</TABLE>
					 </xsl:when>
					 <xsl:otherwise>
						<TABLE id="CutSetTable" cellSpacing="0">
							<TR class="HeaderRow">
								<xsl:for-each select="Objectives/*">
									<TD>
                    <xsl:element name="a">
                      <xsl:attribute name="href">
                        javascript:
                        var parameters = new Array();
                        var parameter = new Object();
                        parameter['name'] = 'xmlFile';
                        parameter['value'] = '<xsl:value-of select="$xmlFile"/>';
                        parameters.push(parameter);

                        parameter = new Object();
                        parameter.name = 'sortType';
                        parameter.value = '<xsl:value-of select="Name" />';

                        parameters.push(parameter);
                        parameter = new Object();
                        parameter.name = 'sortOrder';
                        parameter.value = 'ascending';
                        if (sortOrderParam == 'ascending')
                        {
                        parameter.value = 'descending';
                        }
                        parameters.push(parameter);

                        runTransform('<xsl:value-of select="$xmlFile" />','FTOutput/XSL/ASILOptimisation.xsl', 'ContentSpace', parameters);

                      </xsl:attribute>
                      <xsl:value-of select="Name" />
                    </xsl:element>
									</TD>
								</xsl:for-each>	
								<TD>
									Configuration
								</TD>
							</TR>
							<xsl:for-each select="NonDominatedPopulation/Individual">
                <xsl:sort select="Evaluations/Evaluation[Name[.='{$sortType}']]/Value" order="{$sortOrder}" data-type="number"/>
								<TR class="DataRow">
									<xsl:for-each select="Evaluations/Evaluation">
										<TD>
										<xsl:value-of select="Value" />
									</TD>
									</xsl:for-each>
									<TD>
                    <xsl:element name="a">
                      <xsl:attribute name="href">
                        javascript:
                        var parameter = new Object();
                        parameter.name = 'IndividualIDParam';
                        parameter.value = '<xsl:value-of select="IndividualID" />';
                        var parameters = new Array();
                        parameters.push(parameter);
                        runTransform('<xsl:value-of select="$xmlFile" />','FTOutput/XSL/<xsl:value-of select="local-name(Encoding/*[1])" />.xsl', 'ContentSpace', parameters);
                        runTransform('<xsl:value-of select="$xmlFile" />','FTOutput/XSL/ASILOptimisationSummary.xsl', 'TopEventData', parameters);



                      </xsl:attribute>
                      Click here to see configuration
                    </xsl:element>
									</TD>
								</TR>
							</xsl:for-each>	
						</TABLE>
					 </xsl:otherwise>
				 </xsl:choose>
				</TD>
				</TR>
					</TABLE>


</xsl:template>

  <xsl:template match="CoitGA">
    <TABLE id="ContentTable" cellSpacing="0">
      <TR>
        <TD>
          <xsl:choose>
            <xsl:when test="Population">
              <TABLE id="CutSetTable" cellSpacing="0">
                <TR class="HeaderRow">
                  <xsl:for-each select="Objectives/Objective">
                    <TD>
                      <xsl:element name="a">
                        <xsl:attribute name="href">
                          javascript:
                          var sortOrderParam = '<xsl:value-of select="$sortOrder" />';
                          var parameters = new Array();
                          var parameter = new Object();
                          parameter['name'] = 'xmlFile';
                          parameter['value'] = '<xsl:value-of select="$xmlFile"/>';
                          parameters.push(parameter);

                          parameter = new Object();
                          parameter.name = 'sortType';
                          parameter.value = '<xsl:value-of select="Name" />';

                          parameters.push(parameter);
                          parameter = new Object();
                          parameter.name = 'sortOrder';
                          parameter.value = 'ascending';
                          if (sortOrderParam == 'ascending')
                          {
                          parameter.value = 'descending';
                          }
                          parameters.push(parameter);

                          runTransform('<xsl:value-of select="$xmlFile" />','FTOutput/XSL/ASILOptimisation.xsl', 'ContentSpace', parameters);

                        </xsl:attribute>
                        <xsl:value-of select="Name" />
                      </xsl:element>
                    </TD>
                  </xsl:for-each>
                  <TD>
                    Configuration
                  </TD>
                </TR>
                <xsl:for-each select="Population/Individual">
                  <xsl:sort select="Evaluations/Evaluation[Name[.=$sortType]]/Value" order="{$sortOrder}" data-type="number"/>
                  <TR class="DataRow">
                    <xsl:for-each select="Evaluations/Evaluation">
                      <TD>
                        <xsl:value-of select="Value" />
                      </TD>
                    </xsl:for-each>
                    <TD>
                      <xsl:element name="a">
                        <xsl:attribute name="href">
                          javascript:
                          var parameter = new Object();
                          parameter.name = 'IndividualIDParam';
                          parameter.value = '<xsl:value-of select="IndividualID" />';
                          var parameters = new Array();
                          parameters.push(parameter);
                          runTransform('<xsl:value-of select="$xmlFile" />','FTOutput/XSL/<xsl:value-of select="local-name(Encoding/*[1])" />.xsl', 'ContentSpace', parameters);
                          runTransform('<xsl:value-of select="$xmlFile" />','FTOutput/XSL/ASILOptimisationSummary.xsl', 'TopEventData', parameters);



                        </xsl:attribute>
                        Click here to see configuration
                      </xsl:element>
                    </TD>
                  </TR>
                </xsl:for-each>
              </TABLE>
            </xsl:when>
            <xsl:otherwise>
              <TABLE id="CutSetTable" cellSpacing="0">
                <TR class="HeaderRow">
                  <xsl:for-each select="Objectives/*">
                    <TD>
                      <xsl:element name="a">
                        <xsl:attribute name="href">
                          javascript:
                          var parameters = new Array();
                          var parameter = new Object();
                          parameter['name'] = 'xmlFile';
                          parameter['value'] = '<xsl:value-of select="$xmlFile"/>';
                          parameters.push(parameter);

                          parameter = new Object();
                          parameter.name = 'sortType';
                          parameter.value = '<xsl:value-of select="Name" />';

                          parameters.push(parameter);
                          parameter = new Object();
                          parameter.name = 'sortOrder';
                          parameter.value = 'ascending';
                          if (sortOrderParam == 'ascending')
                          {
                          parameter.value = 'descending';
                          }
                          parameters.push(parameter);

                          runTransform('<xsl:value-of select="$xmlFile" />','FTOutput/XSL/ASILOptimisation.xsl', 'ContentSpace', parameters);

                        </xsl:attribute>
                        <xsl:value-of select="Name" />
                      </xsl:element>
                    </TD>
                  </xsl:for-each>
                  <TD>
                    Configuration
                  </TD>
                </TR>
                <xsl:for-each select="Population/Individual">
                  <xsl:sort select="Evaluations/Evaluation[Name[.='{$sortType}']]/Value" order="{$sortOrder}" data-type="number"/>
                  <TR class="DataRow">
                    <xsl:for-each select="Evaluations/Evaluation">
                      <TD>
                        <xsl:value-of select="Value" />
                      </TD>
                    </xsl:for-each>
                    <TD>
                      <xsl:element name="a">
                        <xsl:attribute name="href">
                          javascript:
                          var parameter = new Object();
                          parameter.name = 'IndividualIDParam';
                          parameter.value = '<xsl:value-of select="IndividualID" />';
                          var parameters = new Array();
                          parameters.push(parameter);
                          runTransform('<xsl:value-of select="$xmlFile" />','FTOutput/XSL/<xsl:value-of select="local-name(Encoding/*[1])" />.xsl', 'ContentSpace', parameters);
                          runTransform('<xsl:value-of select="$xmlFile" />','FTOutput/XSL/ASILOptimisationSummary.xsl', 'TopEventData', parameters);



                        </xsl:attribute>
                        Click here to see configuration
                      </xsl:element>
                    </TD>
                  </TR>
                </xsl:for-each>
              </TABLE>
            </xsl:otherwise>
          </xsl:choose>
        </TD>
      </TR>
    </TABLE>


  </xsl:template>


  <xsl:template match="HiP-HOPS_Results/ExhaustiveSafetyAllocations">
    <TABLE id="ContentTable" cellSpacing="0">
      <TR>
        <TD>
          <xsl:choose>
            <xsl:when test="Population">
              <TABLE id="CutSetTable" cellSpacing="0">
                <TR class="HeaderRow">
                  <xsl:for-each select="Objectives/Objective">
                    <TD>
                      <xsl:element name="a">
                        <xsl:attribute name="href">
                          javascript:
                          var sortOrderParam = '<xsl:value-of select="$sortOrder" />';
                          var parameters = new Array();
                          var parameter = new Object();
                          parameter['name'] = 'xmlFile';
                          parameter['value'] = '<xsl:value-of select="$xmlFile"/>';
                          parameters.push(parameter);

                          parameter = new Object();
                          parameter.name = 'sortType';
                          parameter.value = '<xsl:value-of select="Name" />';

                          parameters.push(parameter);
                          parameter = new Object();
                          parameter.name = 'sortOrder';
                          parameter.value = 'ascending';
                          if (sortOrderParam == 'ascending')
                          {
                          parameter.value = 'descending';
                          }
                          parameters.push(parameter);

                          runTransform('<xsl:value-of select="$xmlFile" />','FTOutput/XSL/ASILOptimisation.xsl', 'ContentSpace', parameters);

                        </xsl:attribute>
                        <xsl:value-of select="Name" />
                      </xsl:element>
                    </TD>
                  </xsl:for-each>
                  <TD>
                    Configuration
                  </TD>
                </TR>
                <xsl:for-each select="Population/Individual">
                  <xsl:sort select="Evaluations/Evaluation[Name[.=$sortType]]/Value" order="{$sortOrder}" data-type="number"/>
                  <TR class="DataRow">
                    <xsl:for-each select="Evaluations/Evaluation">
                      <TD>
                        <xsl:value-of select="Value" />
                      </TD>
                    </xsl:for-each>
                    <TD>
                      <xsl:element name="a">
                        <xsl:attribute name="href">
                          javascript:
						  transformOptimisationIndividual('<xsl:value-of select="$xmlFile" />','FTOutput/XSL/<xsl:value-of select="local-name(Encoding/*[1])" />.xsl', '<xsl:value-of select="IndividualID" />');
                          
                          transformOptimisationIndividualSummary('<xsl:value-of select="$xmlFile" />','FTOutput/XSL/ASILOptimisationSummary.xsl', '<xsl:value-of select="IndividualID" />');



                        </xsl:attribute>
                        Click here to see configuration
                      </xsl:element>
                    </TD>
                  </TR>
                </xsl:for-each>
              </TABLE>
            </xsl:when>
            <xsl:otherwise>
              <TABLE id="CutSetTable" cellSpacing="0">
                <TR class="HeaderRow">
                  <xsl:for-each select="Objectives/*">
                    <TD>
                      <xsl:element name="a">
                        <xsl:attribute name="href">
                          javascript:
                          var parameters = new Array();
                          var parameter = new Object();
                          parameter['name'] = 'xmlFile';
                          parameter['value'] = '<xsl:value-of select="$xmlFile"/>';
                          parameters.push(parameter);

                          parameter = new Object();
                          parameter.name = 'sortType';
                          parameter.value = '<xsl:value-of select="Name" />';

                          parameters.push(parameter);
                          parameter = new Object();
                          parameter.name = 'sortOrder';
                          parameter.value = 'ascending';
                          if (sortOrderParam == 'ascending')
                          {
                          parameter.value = 'descending';
                          }
                          parameters.push(parameter);

                          runTransform('<xsl:value-of select="$xmlFile" />','FTOutput/XSL/ASILOptimisation.xsl', 'ContentSpace', parameters);

                        </xsl:attribute>
                        <xsl:value-of select="Name" />
                      </xsl:element>
                    </TD>
                  </xsl:for-each>
                  <TD>
                    Configuration
                  </TD>
                </TR>
                <xsl:for-each select="Population/Individual">
                  <xsl:sort select="Evaluations/Evaluation[Name[.='{$sortType}']]/Value" order="{$sortOrder}" data-type="number"/>
                  <TR class="DataRow">
                    <xsl:for-each select="Evaluations/Evaluation">
                      <TD>
                        <xsl:value-of select="Value" />
                      </TD>
                    </xsl:for-each>
                    <TD>
                      <xsl:element name="a">
                        <xsl:attribute name="href">
                          javascript:
                         transformOptimisationIndividual('<xsl:value-of select="$xmlFile" />','FTOutput/XSL/<xsl:value-of select="local-name(Encoding/*[1])" />.xsl', '<xsl:value-of select="IndividualID" />');



                        </xsl:attribute>
                        Click here to see configuration
                      </xsl:element>
                    </TD>
                  </TR>
                </xsl:for-each>
              </TABLE>
            </xsl:otherwise>
          </xsl:choose>
        </TD>
      </TR>
    </TABLE>


  </xsl:template>
</xsl:stylesheet>