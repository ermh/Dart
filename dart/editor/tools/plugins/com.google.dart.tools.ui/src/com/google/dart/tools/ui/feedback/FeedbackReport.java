/*
 * Copyright (c) 2011, the Dart project authors.
 * 
 * Licensed under the Eclipse Public License v1.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 * 
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */
package com.google.dart.tools.ui.feedback;

import com.google.dart.tools.ui.DartToolsPlugin;
import com.google.dart.tools.ui.util.PrintStringWriter;

/**
 * An object representing a user feedback report.
 */
public class FeedbackReport {

  private String feedbackText;
  private final String osDetails;
  private final String ideVersion;
  private final String logContents;

  /**
   * Create a new feedback instance with default values.
   */
  public FeedbackReport() {
    this("", FeedbackUtils.getOSName(), DartToolsPlugin.getVersionString(),
        LogReader.readLogSafely());
  }

  /**
   * Create a new feedback instance.
   * 
   * @param feedbackText the user feedback
   * @param ideVersion IDE version
   * @param osDetails OS details
   * @param logContents system log contents
   */
  public FeedbackReport(String feedbackText, String osDetails, String ideVersion, String logContents) {
    this.feedbackText = feedbackText;
    this.osDetails = osDetails;
    this.ideVersion = ideVersion;
    this.logContents = logContents;
  }

  /**
   * Get any additional details that will be sent with this feedback report if the user chooses to
   * opt-in.
   * 
   * @return a detail string suitable for preview
   */
  public String getDetailString() {
    PrintStringWriter writer = new PrintStringWriter();
    new FeedbackWriter(this).writeDetails(writer);
    writer.flush();
    return writer.toString();
  }

  /**
   * Update the user supplied feedback text.
   * 
   * @param feedbackText the updated feedback text
   */
  public void setFeedbackText(String feedbackText) {
    this.feedbackText = feedbackText;
  }

  String getEditorVersion() {
    return ideVersion;
  }

  String getFeedbackText() {
    return feedbackText;
  }

  String getLogContents() {
    return logContents;
  }

  String getOsDetails() {
    return osDetails;
  }

}
