using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Windows.Forms;

namespace SSCMods.Setup {
    public static class Release {
        public static Package Package() {
#if MENU_ALPHA
            var assembly=Assembly.GetExecutingAssembly();
            var package=new Package {RuntimeValidated=true,GameSize=15272960,
                GameHash="959319A3592DE2AED18E398C425FCC79D784ED038DAFD138B361A7D219F56D5E"};
            foreach(var name in new[]{"opengl32.dll","runtime.dll"}) {
                using(var stream=assembly.GetManifestResourceStream(name))
                using(var bytes=new MemoryStream()) {
                    if(stream==null) throw new IOException("Embedded runtime is missing.");
                    stream.CopyTo(bytes);
                    package.Files.Add(new Payload {Path=name=="runtime.dll"?"SSCMods/runtime.dll":name,Bytes=bytes.ToArray()});
                }
            }
            package.Files.Add(new Payload {Path="SSCMods/Uninstall.exe",Bytes=File.ReadAllBytes(assembly.Location)});
            return package;
#else
            return new Package {RuntimeValidated=false};
#endif
        }
    }
    static class Theme {
        public static readonly Color Background=Color.FromArgb(15,28,46), Panel=Color.FromArgb(27,47,73),
            Raised=Color.FromArgb(38,64,96), Blue=Color.FromArgb(31,117,215), Cyan=Color.FromArgb(77,196,255),
            Ink=Color.FromArgb(229,237,250), Muted=Color.FromArgb(164,187,217);
    }
    sealed class CutButton : Button {
        public bool Selected;
        bool pressed;
        bool hover;
        public CutButton(){FlatStyle=FlatStyle.Flat;FlatAppearance.BorderSize=0;Cursor=Cursors.Hand;Font=new Font("Bahnschrift",10,FontStyle.Bold);SetStyle(ControlStyles.UserPaint|ControlStyles.AllPaintingInWmPaint|ControlStyles.OptimizedDoubleBuffer,true);}
        protected override void OnMouseEnter(EventArgs e){hover=true;Invalidate();base.OnMouseEnter(e);}
        protected override void OnMouseLeave(EventArgs e){hover=false;Invalidate();base.OnMouseLeave(e);}
        protected override void OnMouseDown(MouseEventArgs e){pressed=true;Invalidate();base.OnMouseDown(e);}
        protected override void OnMouseUp(MouseEventArgs e){pressed=false;Invalidate();base.OnMouseUp(e);}
        protected override void OnPaint(PaintEventArgs e){
            e.Graphics.Clear(Parent.BackColor);int w=Width-1,h=Height-1,c=8;
            var points=new[]{new Point(c,0),new Point(w-c,0),new Point(w,c),new Point(w,h-c),new Point(w-c,h),new Point(c,h),new Point(0,h-c),new Point(0,c)};
            Color fill=!Enabled?Theme.Panel:pressed?Color.FromArgb(25,90,115):Selected?(hover?Color.FromArgb(100,231,240):Color.FromArgb(64,207,222)):hover?Color.FromArgb(51,85,126):Theme.Raised;
            using(var brush=new SolidBrush(fill))e.Graphics.FillPolygon(brush,points);
            using(var pen=new Pen(Selected?Theme.Cyan:Theme.Raised))e.Graphics.DrawLine(pen,c,0,w-c,0);
            TextRenderer.DrawText(e.Graphics,Text,Font,ClientRectangle,Enabled?(Selected?Theme.Background:Theme.Ink):Theme.Muted,TextFormatFlags.HorizontalCenter|TextFormatFlags.VerticalCenter|TextFormatFlags.EndEllipsis);
            if(Focused&&ShowFocusCues)ControlPaint.DrawFocusRectangle(e.Graphics,Rectangle.Inflate(ClientRectangle,-5,-5),Theme.Cyan,fill);
        }
    }
    public sealed class SetupWindow : Form {
        readonly TextBox folder=new TextBox();
        readonly Label status=new Label();
        readonly CutButton action=new CutButton(),remove=new CutButton(),browse=new CutButton(),close=new CutButton();
        readonly LinkLabel repair=new LinkLabel();
        readonly Timer debounce=new Timer{Interval=450};
        readonly ToolTip tip=new ToolTip();
        readonly Package package;
        readonly bool production;
        bool busy,launchReady;Point dragOrigin;bool dragging;
        public bool Busy {get{return busy;}}
        public SetupWindow(string root,bool uninstall):this(root,uninstall,Release.Package()){production=true;}
        internal SetupWindow(string root,bool uninstall,Package payload){
            package=payload;Text="SSC Mod Menu";
            ClientSize=new Size(600,280);FormBorderStyle=FormBorderStyle.None;MaximizeBox=false;
            AutoScaleMode=AutoScaleMode.Dpi;Font=new Font("Bahnschrift",10);BackColor=Theme.Background;ForeColor=Theme.Ink;
            StartPosition=FormStartPosition.CenterScreen;DoubleBuffered=true;
            var header=new Label{Text="SSC Mod Menu",TextAlign=ContentAlignment.MiddleCenter,BackColor=Theme.Panel,Font=new Font("Bahnschrift",22,FontStyle.Bold)};
            header.SetBounds(0,0,600,70);Controls.Add(header);
            header.MouseDown+=(s,e)=>{if(e.Button==MouseButtons.Left){dragging=true;dragOrigin=Cursor.Position;header.Capture=true;}};
            header.MouseMove+=(s,e)=>{if(dragging){var point=Cursor.Position;Location=new Point(Left+point.X-dragOrigin.X,Top+point.Y-dragOrigin.Y);dragOrigin=point;}};
            header.MouseUp+=(s,e)=>{dragging=false;header.Capture=false;};
            var rule=new Panel{BackColor=Theme.Cyan};rule.SetBounds(0,69,600,2);Controls.Add(rule);rule.BringToFront();
            Controls.Add(new Label{Text="GAME FOLDER",ForeColor=Theme.Muted,Left=24,Top=90,Width=300,Height=22});
            var pathPanel=new Panel{BackColor=Theme.Panel};pathPanel.SetBounds(24,116,440,36);Controls.Add(pathPanel);
            folder.Name="folder";folder.BorderStyle=BorderStyle.None;folder.BackColor=Theme.Panel;folder.ForeColor=Theme.Ink;folder.SetBounds(10,9,420,23);folder.Text=root??"";pathPanel.Controls.Add(folder);
            folder.AccessibleName="Skillshot City game folder";
            browse.Name="browse";browse.Text="BROWSE";browse.SetBounds(476,116,100,36);browse.Click+=(s,e)=>{
                using(var dialog=new FolderBrowserDialog()){dialog.Description="Choose the Skillshot City game folder";dialog.SelectedPath=folder.Text;if(dialog.ShowDialog(this)==DialogResult.OK){folder.Text=dialog.SelectedPath;Check();}}
            };Controls.Add(browse);
            repair.Name="repair";repair.Text="REPAIR";repair.TextAlign=ContentAlignment.MiddleCenter;repair.SetBounds(476,157,100,26);
            repair.LinkColor=Theme.Muted;repair.ActiveLinkColor=Theme.Cyan;repair.VisitedLinkColor=Theme.Muted;
            repair.LinkClicked+=(s,e)=>Execute(2);Controls.Add(repair);
            tip.SetToolTip(repair,"Restore mod files or install if missing. Keeps your settings.");
            status.Name="status";status.SetBounds(24,160,436,42);status.ForeColor=Theme.Cyan;Controls.Add(status);
            action.Name="action";action.Text="INSTALL";action.Selected=true;action.SetBounds(24,216,180,40);action.Click+=(s,e)=>{if(launchReady)Launch();else Execute(0);};Controls.Add(action);
            remove.Name="remove";remove.Text="UNINSTALL";remove.SetBounds(222,216,180,40);remove.Click+=(s,e)=>Execute(1);Controls.Add(remove);
            close.Name="close";close.Text="CLOSE";close.SetBounds(476,216,100,40);close.Click+=(s,e)=>Close();Controls.Add(close);
            AcceptButton=action;CancelButton=close;
            folder.TextChanged+=(s,e)=>{if(busy)return;launchReady=false;action.Enabled=remove.Enabled=false;status.Text="Checking game folder...";debounce.Stop();debounce.Start();};
            debounce.Tick+=(s,e)=>{debounce.Stop();Check();};
            FormClosing+=(s,e)=>{if(busy)e.Cancel=true;};
            Shown+=(s,e)=>Check();
        }
        protected override void OnPaint(PaintEventArgs e){base.OnPaint(e);using(var pen=new Pen(Theme.Raised))e.Graphics.DrawRectangle(pen,0,0,ClientSize.Width-1,ClientSize.Height-1);}
        protected override void Dispose(bool disposing){if(disposing){debounce.Dispose();tip.Dispose();}base.Dispose(disposing);}
        void SetBusy(bool value){busy=value;folder.Enabled=browse.Enabled=repair.Enabled=close.Enabled=!value;action.Enabled=remove.Enabled=false;UseWaitCursor=value;}
        void Status(string value,bool error=false){status.Text=value;status.ForeColor=error?Color.FromArgb(255,196,99):Theme.Cyan;tip.SetToolTip(status,value);}
        async void Check(){
            if(busy)return;launchReady=false;debounce.Stop();SetBusy(true);Status("Checking game folder...");
            try{
                string path=folder.Text;
                if(String.IsNullOrWhiteSpace(path)){path=await System.Threading.Tasks.Task.Run(()=>Engine.Discover());folder.Text=path;}
                bool owned=false,ready=false;string message="";
                await System.Threading.Tasks.Task.Run(()=>{
                    owned=Engine.HasInstallation(path);
                    try{var result=owned?Engine.InspectUpdate(path,package):Engine.Inspect(path,package);ready=result.CanInstall;message=result.Message;}
                    catch(Exception error){message=error.Message;}
                });
                SetBusy(false);action.Text=owned?"UPDATE":"INSTALL";action.Enabled=ready;remove.Enabled=owned;
                Status(ready?"Compatible game found":message,!ready);
            }catch(Exception error){SetBusy(false);Status(error.Message,true);}
        }
        void Launch(){
            if(busy)return;
            try{
                string root=Path.GetFullPath(folder.Text);
                if(!Engine.HasInstallation(root))throw new IOException("Install the mod before launching.");
                string steam=Engine.Discover();
                bool steamGame=!String.IsNullOrEmpty(steam)&&String.Equals(root.TrimEnd('\\'),Path.GetFullPath(steam).TrimEnd('\\'),StringComparison.OrdinalIgnoreCase);
                Process.Start(new ProcessStartInfo(steamGame?"steam://rungameid/308600":Path.Combine(root,"SkillshotCity.exe")){UseShellExecute=true,WorkingDirectory=root});
                Status("Launching Skillshot City...");
            }catch(Exception error){Status(error.Message,true);}
        }
        public void RefreshInspection(){Check();}
        async void Execute(int operation){
            if(busy)return;string path=folder.Text;debounce.Stop();SetBusy(true);
            Status(operation==1?"Uninstalling...":operation==2?"Repairing...":"Installing...");
            try{
                string message;
                if(production&&Updater.NeedsElevation(path)) {
                    message=await System.Threading.Tasks.Task.Run(()=>{
                        string job=Path.Combine(Path.GetTempPath(),"SSCMods-Action-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(job);
                        string args="--operation "+operation+" "+Updater.Quote(path)+" "+Updater.Quote(job);
                        using(var child=Process.Start(new ProcessStartInfo(Assembly.GetExecutingAssembly().Location,args){UseShellExecute=true,Verb="runas",WindowStyle=ProcessWindowStyle.Hidden})){
                            child.WaitForExit();string outcome=Path.Combine(job,"status.txt");string result=File.Exists(outcome)?File.ReadAllText(outcome):"Installer did not complete.";
                            if(child.ExitCode!=0)throw new IOException(result);return result;
                        }
                    });
                }
                else if(operation==1){var retained=await System.Threading.Tasks.Task.Run(()=>Engine.Uninstall(path));message=retained.Count==0?"SSC Mod Menu uninstalled.":"Some changed files were kept.";}
                else {await System.Threading.Tasks.Task.Run(()=>{if(operation==2)Engine.Repair(path,package);else Engine.InstallOrUpdate(path,package);});message=operation==2?"Repair complete. Settings kept.":"SSC Mod Menu is ready.";}
                SetBusy(false);Status(message);
                bool owned=File.Exists(Path.Combine(path,Engine.ManifestName));launchReady=owned&&operation!=1;action.Text=launchReady?"LAUNCH":owned?"UPDATE":"INSTALL";action.Enabled=true;remove.Enabled=owned;
            }catch(Exception error){SetBusy(false);Status(error.Message,true);}
        }
    }
    public static class Program {
        [STAThread]
        public static int Main(string[] args) {
            Application.EnableVisualStyles(); Application.SetCompatibleTextRenderingDefault(false);
            try {
                if(args.Length==2&&args[0]=="--update-check"){Updater.Check(args[1]);return 0;}
                if(args.Length==6&&args[0]=="--update-download"){Updater.DownloadAndApply(args[1],Int32.Parse(args[2]),Int64.Parse(args[3]),args[4],args[5]);return 0;}
                if(args.Length==5&&args[0]=="--update-wait"){
                    try{Updater.ApplyAfterExit(args[1],Int32.Parse(args[2]),Int64.Parse(args[3]),args[4],Release.Package());return 0;}
                    catch(Exception error){Updater.Report(args[4],"error "+error.Message);return 1;}
                }
                if(args.Length==4&&args[0]=="--operation"){
                    try{
                        string message;
                        if(args[1]=="1")message=Engine.Uninstall(args[2]).Count==0?"SSC Mod Menu uninstalled.":"Some changed files were kept.";
                        else if(args[1]=="2"){Engine.Repair(args[2],Release.Package());message="Repair complete. Settings kept.";}
                        else if(args[1]=="0"){Engine.InstallOrUpdate(args[2],Release.Package());message="SSC Mod Menu is ready.";}
                        else throw new IOException("Invalid installer operation.");
                        Updater.Report(args[3],message);return 0;
                    }catch(Exception error){Updater.Report(args[3],error.Message);return 1;}
                }
                if(args.Length==2&&args[0]=="--apply") {Engine.InstallOrUpdate(args[1],Release.Package());return 0;}
                string exe=Assembly.GetExecutingAssembly().Location;
                // The installed uninstaller runs from a private staging folder so the game-side EXE can be removed.
                if(Path.GetFileName(exe).Equals("Uninstall.exe",StringComparison.OrdinalIgnoreCase)) {
                    string root=Directory.GetParent(Path.GetDirectoryName(exe)).FullName;
                    string temp=Path.Combine(Path.GetTempPath(),"SSCMods-Uninstall-"+Guid.NewGuid().ToString("N"));
                    Directory.CreateDirectory(temp);
                    string helper=Path.Combine(temp,"SSC-Mod-Menu-Setup.exe"); File.Copy(exe,helper,false);
                    Process.Start(new ProcessStartInfo(helper,"--uninstall \""+root+"\" --wait "+Process.GetCurrentProcess().Id) {UseShellExecute=true});
                    return 0;
                }
                bool remove=args.Length>=2 && args[0]=="--uninstall";
                if(args.Length==4 && remove && args[2]=="--wait") {
                    int pid;
                    if(!Int32.TryParse(args[3],out pid)) throw new IOException("Invalid uninstaller process identifier.");
                    try { using(var parent=Process.GetProcessById(pid)) if(!parent.WaitForExit(10000)) throw new IOException("Close the previous uninstaller and retry."); }
                    catch(ArgumentException) {}
                } else if(args.Length!=0 && !(remove&&args.Length==2)) throw new IOException("Usage: SSC-Mod-Menu-Setup.exe [--uninstall GAME_DIRECTORY]");
                Application.Run(new SetupWindow(remove?args[1]:null,remove)); return 0;
            } catch(Exception error) { if(args.Length>0&&(args[0]=="--apply"||args[0].StartsWith("--update-"))) {Console.Error.WriteLine(error);return 1;} MessageBox.Show(error.Message,"SSC Mod Menu",MessageBoxButtons.OK,MessageBoxIcon.Error); return 1; }
        }
    }
}




