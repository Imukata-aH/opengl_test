# opengl_test
OpenGL Tutorial & Some Experiments

### Visual Studioでのプロジェクト作成方法
1. ./OpenGLディレクトリを適当な場所に配置し、そこに環境変数OPEN_GLを通す。
2. さらにOpenGL/binディレクトリにPATHを通す。
3. Visual Studioで作成したプロジェクトに対して、OpenGL.propsを設定する。

### 実行時にGLEW, GLFWでエラーが出る場合
1. glfwまたはglew内のzipを展開して、ビルドする（ビルド方法は各フォルダのReadMe参照）。
2. 作成されたdllをOpenGL/bin内のdllと入れ替える。（OpenGL/include内のヘッダファイルも）
3. これでもうまくいかない場合は、glfwまたはglewのGitHubリポジトリから最新版を落として使う。
