using System;
using System.IO;
using SSCMods.Setup;
class UpdaterFailureTest {
 static void Require(bool ok) {if(!ok)throw new Exception("Updater error propagation regression");}
 static int Main(string[] args) {
  if(args.Length!=1)return 2;
  string parent=Path.GetFullPath(args[0]);Directory.CreateDirectory(parent);
  string job=Path.Combine(parent,"installer-error-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(job);
  string fallback="Update was not installed. See the update status.";
  Require(Updater.InstallerFailure(job)==fallback);
  foreach(string status in new[]{"ready Installer ready","preparing Waiting for installer","downloading Downloading update","error ","noterror no"}) {
   Updater.Report(job,status);Require(Updater.InstallerFailure(job)==fallback);
  }
  foreach(string error in new[]{"Invalid owned-file hash.","This installer does not support the current game version.","The game stayed open. Update cancelled.","Verification failed: \u2713"}) {
   Updater.Report(job,"error "+error);Require(Updater.InstallerFailure(job)==error);
  }
  Updater.Report(job,"error "+new string('x',4097));Require(Updater.InstallerFailure(job)==fallback);
  Updater.Report(job,"error Original installer reason");
  string propagated=Updater.InstallerFailure(job);Updater.Report(job,"error "+propagated);
  Require(File.ReadAllText(Path.Combine(job,"status.txt"))=="error Original installer reason");
  Console.WriteLine("PASS installer errors survive the parent handoff; missing, non-error, empty and oversized status files use a bounded generic message.");return 0;
 }
}
