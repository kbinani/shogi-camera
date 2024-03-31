import UIKit

protocol HelpViewControllerDelegate: AnyObject {
  func helpViewControllerBack(_ vc: HelpViewController)
}

class HelpViewController: UIViewController {
  weak var delegate: HelpViewControllerDelegate?

  private var backButton: RoundButton!
  private var cameraInstructionTitle: UILabel!
  private var cameraInstructionMessage1: UILabel!
  private var cameraInstructionImage: UIImageView!
  private var cameraInstructionMessage2: UILabel!
  private var gameInstructionTitle: UILabel!
  private var gameInstructionMessage1: UILabel!
  private var gameInstructionImage: UIImageView!
  private var gameInstructionMessage2: UILabel!
  private var csaInstructionTitle: UILabel!
  private var csaInstructionMessage1: UITextView!
  private var acknowledgementTitle: UILabel!
  private var acknowledgement: UILabel!
  private var openSourceLicenseTitle: UILabel!
  private var openSourceLicense: UILabel!
  private var container: UIView!
  private var scroll: UIScrollView!

  override func viewDidLoad() {
    super.viewDidLoad()

    self.view.backgroundColor = UIColor.init(white: 0.2, alpha: 1)

    let container = UIView()
    let titleScale: CGFloat = 1.2

    let backButton = RoundButton(type: .custom)
    backButton.setTitle("戻る", for: .normal)
    backButton.addTarget(self, action: #selector(backButtonDidTouchUpInside(_:)), for: .touchUpInside)
    self.view.addSubview(backButton)
    self.backButton = backButton

    let cameraInstructionTitle = UILabel()
    let font = cameraInstructionTitle.font!
    let titleFont = UIFont.boldSystemFont(ofSize: font.pointSize * titleScale)
    cameraInstructionTitle.text = "カメラの設定方法"
    cameraInstructionTitle.font = titleFont
    cameraInstructionTitle.textAlignment = .center
    cameraInstructionTitle.textColor = .white
    container.addSubview(cameraInstructionTitle)
    self.cameraInstructionTitle = cameraInstructionTitle

    let cameraInstructionMessage1 = UILabel()
    cameraInstructionMessage1.text = "　将棋盤をなるべく真上から撮影できる位置にデバイスを固定します。将棋盤の位置が認識できると、対局開始ボタンが押せるようになるので、手番を選んで対局開始ボタンを押します。"
    cameraInstructionMessage1.numberOfLines = 0
    cameraInstructionMessage1.lineBreakMode = .byWordWrapping
    cameraInstructionMessage1.textColor = .white
    container.addSubview(cameraInstructionMessage1)
    self.cameraInstructionMessage1 = cameraInstructionMessage1

    let cameraInstructionImage = UIImageView()
    cameraInstructionImage.image = .init(named: "cameraInstructionImage")
    cameraInstructionImage.contentMode = .scaleAspectFit
    cameraInstructionImage.layer.borderWidth = 1
    cameraInstructionImage.layer.borderColor = UIColor.black.cgColor
    container.addSubview(cameraInstructionImage)
    self.cameraInstructionImage = cameraInstructionImage

    let cameraInstructionMessage2 = UILabel()
    cameraInstructionMessage2.text = "　上の図は実際の設定例です。緑色の枠は、本アプリが認識した将棋盤の枠を表しています。この枠が安定して正確な位置に来るようにカメラの位置や照明を変えてみて下さい。"
    cameraInstructionMessage2.numberOfLines = 0
    cameraInstructionMessage2.lineBreakMode = .byWordWrapping
    cameraInstructionMessage2.textColor = .white
    container.addSubview(cameraInstructionMessage2)
    self.cameraInstructionMessage2 = cameraInstructionMessage2

    let gameInstructionTitle = UILabel()
    gameInstructionTitle.text = "対局方法"
    gameInstructionTitle.font = titleFont
    gameInstructionTitle.textAlignment = .center
    gameInstructionTitle.textColor = .white
    container.addSubview(gameInstructionTitle)
    self.gameInstructionTitle = gameInstructionTitle

    let gameInstructionMessage1 = UILabel()
    // swift-format-ignore
    gameInstructionMessage1.text =
"""
　カメラの設定が完了すると対局開始ボタンが押せる状態になります。先手番で指すか、後手番で指すかを選んで対局開始ボタンを押して下さい。
　プレイヤーの指し手が認識されるとその符号をアプリが読み上げます。続いて将棋 AI が指し手を決めて符号を読み上げるので、その通りに AI 側の駒を動かして下さい。AI 側の駒の移動が完了したと認識されると「ピポ」と音が鳴るので、その後に自分側の次の手に着手して下さい。
"""

    gameInstructionMessage1.numberOfLines = 0
    gameInstructionMessage1.lineBreakMode = .byWordWrapping
    gameInstructionMessage1.textColor = .white
    container.addSubview(gameInstructionMessage1)
    self.gameInstructionMessage1 = gameInstructionMessage1

    let gameInstructionImage = UIImageView()
    gameInstructionImage.image = .init(named: "gameInstructionImage")
    gameInstructionImage.contentMode = .scaleAspectFit
    gameInstructionImage.layer.borderWidth = 1
    gameInstructionImage.layer.borderColor = UIColor.black.cgColor
    container.addSubview(gameInstructionImage)
    self.gameInstructionImage = gameInstructionImage

    let gameInstructionMessage2 = UILabel()
    // swift-format-ignore
    gameInstructionMessage2.text =
"""
　上の図は対局画面の例です。対局画面には画面上部に盤面の図、画面下部左に棋譜、下部右にカメラがキャプチャしている盤面の様子が表示されます。対局後は左下の「kifuエクスポート」ボタンから棋譜を kifu 形式でエクスポートできます。
　対局を中断する場合や投了する場合は画面上部のボタンから行って下さい。
"""
    gameInstructionMessage2.numberOfLines = 0
    gameInstructionMessage2.lineBreakMode = .byWordWrapping
    gameInstructionMessage2.textColor = .white
    container.addSubview(gameInstructionMessage2)
    self.gameInstructionMessage2 = gameInstructionMessage2

    let csaInstructionTitle = UILabel()
    csaInstructionTitle.text = "通信対局モードについて"
    csaInstructionTitle.font = titleFont
    csaInstructionTitle.textAlignment = .center
    csaInstructionTitle.textColor = .white
    container.addSubview(csaInstructionTitle)
    self.csaInstructionTitle = csaInstructionTitle

    let csaInstructionMessage1 = UITextView()
    let csaInstructionMessage = NSMutableAttributedString()
    let indent = NSAttributedString(string: "　", attributes: [.font: font]).size().width
    let listIndent = NSAttributedString(string: "① ", attributes: [.font: font]).size().width + indent
    csaInstructionMessage.append(paragraph(font, text: "世界コンピュータ将棋選手権で用いられ、広く利用されている CSA 通信プロトコルを使用して通信対局を行うモードです。CSA プロトコルに対応した将棋ソフトを PC で起動し、本アプリに内蔵されている CSA サーバーと接続して通信対局を行います。", indent: 0, headIndent: indent))
    csaInstructionMessage.append(paragraph(font, text: "", indent: 0, headIndent: 0))
    csaInstructionMessage.append(paragraph(font, text: "通信対局を行うには:", indent: 0, headIndent: indent))
    csaInstructionMessage.append(paragraph(font, text: "", indent: 0, headIndent: 0))
    csaInstructionMessage.append(paragraph(font, text: "① 本アプリを起動し、WiFi 接続を ON にします。", indent: listIndent, headIndent: indent))
    csaInstructionMessage.append(paragraph(font, text: "② PC を iPhone / iPad と同じ LAN に接続し、将棋ソフトを起動します。", indent: listIndent, headIndent: indent))
    csaInstructionMessage.append(paragraph(font, text: "③ 本アプリ画面下部の「通信対局モード」を ON にします。CSA サーバーの起動が完了すると画面下部に「アドレス: 192.168.x.x, ポート: xxxx」などと表示されるのでメモしておきます。", indent: listIndent, headIndent: indent))
    csaInstructionMessage.append(paragraph(font, text: "④ 将棋ソフトで通信対局の設定画面で、メモしておいたアドレスとポートを入力し、対局開始します。", indent: listIndent, headIndent: indent))
    csaInstructionMessage.append(paragraph(font, text: "⑤ 本アプリに戻り、将棋盤が映るようにデバイスを固定して対局開始ボタンが押せる状態になるのを待ちます。", indent: listIndent, headIndent: indent))
    csaInstructionMessage.append(paragraph(font, text: "⑥ 対局開始ボタンが押せる状態になったら、手番を選んで対局開始ボタンを押します。以降は、通信対局 OFF の場合と同様に対局を行います。", indent: listIndent, headIndent: indent))
    csaInstructionMessage.append(paragraph(font, text: "", indent: 0, headIndent: 0))
    let text = NSMutableAttributedString(attributedString: paragraph(font, text: "将棋ソフト毎の具体的な設定方法は将棋ソフト毎の通信対局方法の詳細を参照して下さい", indent: 0, headIndent: 0))
    let linkRange = (text.string as NSString).range(of: "将棋ソフト毎の通信対局方法の詳細")
    text.addAttribute(.link, value: "https://scrapbox.io/shogi-camera/%E5%B0%86%E6%A3%8B%E3%82%BD%E3%83%95%E3%83%88%E6%AF%8E%E3%81%AE%E9%80%9A%E4%BF%A1%E5%AF%BE%E5%B1%80%E6%96%B9%E6%B3%95%E3%81%AE%E8%A9%B3%E7%B4%B0", range: linkRange)
    csaInstructionMessage.append(text)
    csaInstructionMessage1.attributedText = csaInstructionMessage
    csaInstructionMessage1.isEditable = false
    csaInstructionMessage1.isScrollEnabled = false
    csaInstructionMessage1.delegate = self
    csaInstructionMessage1.backgroundColor = .white.withAlphaComponent(0)
    csaInstructionMessage1.clipsToBounds = false
    container.addSubview(csaInstructionMessage1)
    self.csaInstructionMessage1 = csaInstructionMessage1

    let acknowledgementTitle = UILabel()
    acknowledgementTitle.text = "謝辞"
    acknowledgementTitle.font = titleFont
    acknowledgementTitle.textAlignment = .center
    acknowledgementTitle.textColor = .white
    container.addSubview(acknowledgementTitle)
    self.acknowledgementTitle = acknowledgementTitle

    let acknowledgement = UILabel()
    acknowledgement.numberOfLines = 0
    acknowledgement.lineBreakMode = .byWordWrapping
    acknowledgement.textAlignment = .center
    acknowledgement.textColor = .white
    // swift-format-ignore
    acknowledgement.text = """
棋譜読み上げボイスとして
「VOICEVOX:ずんだもん」
を利用させていただきました。感謝申し上げます。
"""
    container.addSubview(acknowledgement)
    self.acknowledgement = acknowledgement

    let openSourceLicenseTitle = UILabel()
    openSourceLicenseTitle.text = "オープンソースライセンス"
    openSourceLicenseTitle.font = titleFont
    openSourceLicenseTitle.textAlignment = .center
    openSourceLicenseTitle.textColor = .white
    container.addSubview(openSourceLicenseTitle)
    self.openSourceLicenseTitle = openSourceLicenseTitle

    let openSourceLicense = UILabel()
    openSourceLicense.textColor = .white
    // swift-format-ignore
    let line1 =
"""
---
OpenCV

                                 Apache License
                           Version 2.0, January 2004
                        http://www.apache.org/licenses/

   TERMS AND CONDITIONS FOR USE, REPRODUCTION, AND DISTRIBUTION

   1. Definitions.

      "License" shall mean the terms and conditions for use, reproduction,
      and distribution as defined by Sections 1 through 9 of this document.

      "Licensor" shall mean the copyright owner or entity authorized by
      the copyright owner that is granting the License.

      "Legal Entity" shall mean the union of the acting entity and all
      other entities that control, are controlled by, or are under common
      control with that entity. For the purposes of this definition,
      "control" means (i) the power, direct or indirect, to cause the
      direction or management of such entity, whether by contract or
      otherwise, or (ii) ownership of fifty percent (50%) or more of the
      outstanding shares, or (iii) beneficial ownership of such entity.

      "You" (or "Your") shall mean an individual or Legal Entity
      exercising permissions granted by this License.

      "Source" form shall mean the preferred form for making modifications,
      including but not limited to software source code, documentation
      source, and configuration files.

      "Object" form shall mean any form resulting from mechanical
      transformation or translation of a Source form, including but
      not limited to compiled object code, generated documentation,
      and conversions to other media types.

      "Work" shall mean the work of authorship, whether in Source or
      Object form, made available under the License, as indicated by a
      copyright notice that is included in or attached to the work
      (an example is provided in the Appendix below).

      "Derivative Works" shall mean any work, whether in Source or Object
      form, that is based on (or derived from) the Work and for which the
      editorial revisions, annotations, elaborations, or other modifications
      represent, as a whole, an original work of authorship. For the purposes
      of this License, Derivative Works shall not include works that remain
      separable from, or merely link (or bind by name) to the interfaces of,
      the Work and Derivative Works thereof.

      "Contribution" shall mean any work of authorship, including
      the original version of the Work and any modifications or additions
      to that Work or Derivative Works thereof, that is intentionally
      submitted to Licensor for inclusion in the Work by the copyright owner
      or by an individual or Legal Entity authorized to submit on behalf of
      the copyright owner. For the purposes of this definition, "submitted"
      means any form of electronic, verbal, or written communication sent
      to the Licensor or its representatives, including but not limited to
      communication on electronic mailing lists, source code control systems,
      and issue tracking systems that are managed by, or on behalf of, the
      Licensor for the purpose of discussing and improving the Work, but
      excluding communication that is conspicuously marked or otherwise
      designated in writing by the copyright owner as "Not a Contribution."

      "Contributor" shall mean Licensor and any individual or Legal Entity
      on behalf of whom a Contribution has been received by Licensor and
      subsequently incorporated within the Work.

   2. Grant of Copyright License. Subject to the terms and conditions of
      this License, each Contributor hereby grants to You a perpetual,
      worldwide, non-exclusive, no-charge, royalty-free, irrevocable
      copyright license to reproduce, prepare Derivative Works of,
      publicly display, publicly perform, sublicense, and distribute the
      Work and such Derivative Works in Source or Object form.

   3. Grant of Patent License. Subject to the terms and conditions of
      this License, each Contributor hereby grants to You a perpetual,
      worldwide, non-exclusive, no-charge, royalty-free, irrevocable
      (except as stated in this section) patent license to make, have made,
      use, offer to sell, sell, import, and otherwise transfer the Work,
      where such license applies only to those patent claims licensable
      by such Contributor that are necessarily infringed by their
      Contribution(s) alone or by combination of their Contribution(s)
      with the Work to which such Contribution(s) was submitted. If You
      institute patent litigation against any entity (including a
      cross-claim or counterclaim in a lawsuit) alleging that the Work
      or a Contribution incorporated within the Work constitutes direct
      or contributory patent infringement, then any patent licenses
      granted to You under this License for that Work shall terminate
      as of the date such litigation is filed.

   4. Redistribution. You may reproduce and distribute copies of the
      Work or Derivative Works thereof in any medium, with or without
      modifications, and in Source or Object form, provided that You
      meet the following conditions:

      (a) You must give any other recipients of the Work or
          Derivative Works a copy of this License; and

      (b) You must cause any modified files to carry prominent notices
          stating that You changed the files; and

      (c) You must retain, in the Source form of any Derivative Works
          that You distribute, all copyright, patent, trademark, and
          attribution notices from the Source form of the Work,
          excluding those notices that do not pertain to any part of
          the Derivative Works; and

      (d) If the Work includes a "NOTICE" text file as part of its
          distribution, then any Derivative Works that You distribute must
          include a readable copy of the attribution notices contained
          within such NOTICE file, excluding those notices that do not
          pertain to any part of the Derivative Works, in at least one
          of the following places: within a NOTICE text file distributed
          as part of the Derivative Works; within the Source form or
          documentation, if provided along with the Derivative Works; or,
          within a display generated by the Derivative Works, if and
          wherever such third-party notices normally appear. The contents
          of the NOTICE file are for informational purposes only and
          do not modify the License. You may add Your own attribution
          notices within Derivative Works that You distribute, alongside
          or as an addendum to the NOTICE text from the Work, provided
          that such additional attribution notices cannot be construed
          as modifying the License.

      You may add Your own copyright statement to Your modifications and
      may provide additional or different license terms and conditions
      for use, reproduction, or distribution of Your modifications, or
      for any such Derivative Works as a whole, provided Your use,
      reproduction, and distribution of the Work otherwise complies with
      the conditions stated in this License.

   5. Submission of Contributions. Unless You explicitly state otherwise,
      any Contribution intentionally submitted for inclusion in the Work
      by You to the Licensor shall be under the terms and conditions of
      this License, without any additional terms or conditions.
      Notwithstanding the above, nothing herein shall supersede or modify
      the terms of any separate license agreement you may have executed
      with Licensor regarding such Contributions.

   6. Trademarks. This License does not grant permission to use the trade
      names, trademarks, service marks, or product names of the Licensor,
      except as required for reasonable and customary use in describing the
      origin of the Work and reproducing the content of the NOTICE file.

   7. Disclaimer of Warranty. Unless required by applicable law or
      agreed to in writing, Licensor provides the Work (and each
      Contributor provides its Contributions) on an "AS IS" BASIS,
      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
      implied, including, without limitation, any warranties or conditions
      of TITLE, NON-INFRINGEMENT, MERCHANTABILITY, or FITNESS FOR A
      PARTICULAR PURPOSE. You are solely responsible for determining the
      appropriateness of using or redistributing the Work and assume any
      risks associated with Your exercise of permissions under this License.

   8. Limitation of Liability. In no event and under no legal theory,
      whether in tort (including negligence), contract, or otherwise,
      unless required by applicable law (such as deliberate and grossly
      negligent acts) or agreed to in writing, shall any Contributor be
      liable to You for damages, including any direct, indirect, special,
      incidental, or consequential damages of any character arising as a
      result of this License or out of the use or inability to use the
      Work (including but not limited to damages for loss of goodwill,
      work stoppage, computer failure or malfunction, or any and all
      other commercial damages or losses), even if such Contributor
      has been advised of the possibility of such damages.

   9. Accepting Warranty or Additional Liability. While redistributing
      the Work or Derivative Works thereof, You may choose to offer,
      and charge a fee for, acceptance of support, warranty, indemnity,
      or other liability obligations and/or rights consistent with this
      License. However, in accepting such obligations, You may act only
      on Your own behalf and on Your sole responsibility, not on behalf
      of any other Contributor, and only if You agree to indemnify,
      defend, and hold each Contributor harmless for any liability
      incurred by, or claims asserted against, such Contributor by reason
      of your accepting any such warranty or additional liability.

   END OF TERMS AND CONDITIONS

   APPENDIX: How to apply the Apache License to your work.

      To apply the Apache License to your work, attach the following
      boilerplate notice, with the fields enclosed by brackets "[]"
      replaced with your own identifying information. (Don't include
      the brackets!)  The text should be enclosed in the appropriate
      comment syntax for the file format. We also recommend that a
      file or class name and description of purpose be included on the
      same "printed page" as the copyright notice for easier
      identification within third-party archives.

   Copyright [yyyy] [name of copyright owner]

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.


"""

    #if SHOGI_CAMERA_DEBUG
      let line2 =
        """
        ---
        sunfish3

        The MIT License (MIT)

        Copyright (c) 2015 Ryosuke Kubo

        Permission is hereby granted, free of charge, to any person obtaining a copy
        of this software and associated documentation files (the "Software"), to deal
        in the Software without restriction, including without limitation the rights
        to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
        copies of the Software, and to permit persons to whom the Software is
        furnished to do so, subject to the following conditions:

        The above copyright notice and this permission notice shall be included in
        all copies or substantial portions of the Software.

        THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
        IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
        FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
        AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
        LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
        OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
        THE SOFTWARE.


        """
    #else
      let line2 = ""
    #endif
    let line3 =
      """
      ---
      base64

      MIT License

      Copyright (c) 2019 Tobias Locker

      Permission is hereby granted, free of charge, to any person obtaining a copy
      of this software and associated documentation files (the "Software"), to deal
      in the Software without restriction, including without limitation the rights
      to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
      copies of the Software, and to permit persons to whom the Software is
      furnished to do so, subject to the following conditions:

      The above copyright notice and this permission notice shall be included in all
      copies or substantial portions of the Software.

      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
      IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
      FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
      AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
      LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
      OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
      SOFTWARE.
      """
    openSourceLicense.text = line1 + line2 + line3
    openSourceLicense.numberOfLines = 0
    openSourceLicense.lineBreakMode = .byWordWrapping
    container.addSubview(openSourceLicense)
    self.openSourceLicense = openSourceLicense

    let scroll = UIScrollView(frame: .zero)
    scroll.addSubview(container)
    scroll.isScrollEnabled = true
    self.scroll = scroll
    self.view.addSubview(scroll)

    self.container = container
  }

  private func measure(_ title: UILabel, width: CGFloat) -> CGSize {
    guard let text = title.text, let font = title.font else {
      return .zero
    }
    let str = NSAttributedString(string: text, attributes: [.font: font])
    return str.boundingRect(
      with: .init(width: width, height: CGFloat.greatestFiniteMagnitude),
      options: [.usesLineFragmentOrigin], context: nil
    ).size
  }

  private func paragraph(_ font: UIFont, text: String, indent: CGFloat, headIndent: CGFloat) -> NSAttributedString {
    let style = NSMutableParagraphStyle()
    style.headIndent = indent
    style.firstLineHeadIndent = headIndent
    return NSAttributedString(
      string: text + "\n",
      attributes: [
        .font: font,
        .paragraphStyle: style,
        .foregroundColor: UIColor.white,
      ])
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    let margin: CGFloat = 15
    let paragraphMargin = margin * 5
    var bounds = CGRect(origin: .zero, size: view.bounds.size)
    bounds.reduce(view.safeAreaInsets)
    bounds.expand(-margin, -margin)

    var backButton = bounds.removeFromTop(44)
    self.backButton.frame = backButton.removeFromLeft(self.backButton.intrinsicContentSize.width)
    bounds.removeFromTop(margin)
    scroll.frame = bounds

    var container = CGRect(x: 0, y: 0, width: bounds.width, height: CGFloat.greatestFiniteMagnitude)
    cameraInstructionTitle.frame = container.removeFromTop(cameraInstructionTitle.intrinsicContentSize.height)
    container.removeFromTop(margin)

    cameraInstructionMessage1.frame = container.removeFromTop(measure(cameraInstructionMessage1, width: container.width).height)

    if let image = cameraInstructionImage.image {
      container.removeFromTop(margin)
      cameraInstructionImage.frame = image.size.aspectFit(within: container.removeFromTop(container.width))
      container.removeFromTop(margin)
      cameraInstructionMessage2.frame = container.removeFromTop(measure(cameraInstructionMessage2, width: container.width).height)
    }
    container.removeFromTop(paragraphMargin)

    gameInstructionTitle.frame = container.removeFromTop(gameInstructionTitle.intrinsicContentSize.height)
    container.removeFromTop(margin)

    gameInstructionMessage1.frame = container.removeFromTop(measure(gameInstructionMessage1, width: container.width).height)

    if let image = gameInstructionImage.image {
      container.removeFromTop(margin)
      gameInstructionImage.frame = image.size.aspectFit(within: container.removeFromTop(container.width))
      container.removeFromTop(margin)
      gameInstructionMessage2.frame = container.removeFromTop(measure(gameInstructionMessage2, width: container.width).height)
    }
    container.removeFromTop(paragraphMargin)

    csaInstructionTitle.frame = container.removeFromTop(csaInstructionTitle.intrinsicContentSize.height)
    container.removeFromTop(margin)

    csaInstructionMessage1.frame = container.removeFromTop(
      csaInstructionMessage1.attributedText.boundingRect(
        with: .init(width: container.width, height: CGFloat.greatestFiniteMagnitude),
        options: [.usesLineFragmentOrigin],
        context: nil
      ).height)
    container.removeFromTop(paragraphMargin)

    acknowledgementTitle.frame = container.removeFromTop(acknowledgementTitle.intrinsicContentSize.height)
    container.removeFromTop(margin)

    acknowledgement.frame = container.removeFromTop(measure(acknowledgement, width: container.width).height)
    container.removeFromTop(paragraphMargin)

    openSourceLicenseTitle.frame = container.removeFromTop(measure(openSourceLicenseTitle, width: container.width).height)
    container.removeFromTop(margin)

    openSourceLicense.frame = container.removeFromTop(measure(openSourceLicense, width: container.width).height)
    container.removeFromTop(margin)

    let containerHeight = container.minY
    self.container.frame = .init(x: 0, y: 0, width: bounds.width, height: containerHeight)
    scroll.contentSize = .init(width: bounds.width, height: containerHeight)
  }

  @objc private func backButtonDidTouchUpInside(_ sender: UIButton) {
    self.delegate?.helpViewControllerBack(self)
  }
}

extension HelpViewController: UITextViewDelegate {
  func textView(_ textView: UITextView, shouldInteractWith url: URL, in characterRange: NSRange, interaction: UITextItemInteraction) -> Bool {
    UIApplication.shared.open(url)
    return false
  }
}
